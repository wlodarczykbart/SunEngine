#include <thread>
#include <condition_variable>

#include "ThreadPool.h"

namespace SunEngine
{
    struct ThreadPool::ThreadData
    {
        ThreadData()
        {
            //threadIndex = 0;
        }

        //uint threadIndex;
        Vector<Pair<TaskCallback, void*>> tasks;
        //Queue<Pair<TaskCallback, void*>> mainThreadTask;
    };

    class ThreadPool::WorkerThread
    {
    public:
        WorkerThread(uint index)
        {
            _data = 0;
            _destroyed = false;
            _index = index;
        }

        void Create(ThreadData* data)
        {
            _data = data;
            _destroyed = false;
            _thread = std::thread(&ThreadPool::WorkerThread::Run, this);
        }

        void Destroy()
        {
            if (_thread.joinable())
            {
                _destroyed = true;
                _condition.notify_one(); //need to call this instead of one since there is a shared condition
                _thread.join();
            }
        }

        void Run()
        {
            while (true)
            {
                {
                    std::unique_lock<std::mutex> lock(_mtx);
                    _condition.wait(lock, [this]() -> bool { return !_tasks.empty() || _destroyed; });
                    if (_destroyed)
                        break;
                }

                while(!_tasks.empty())
                {
                    auto& t = _tasks.front();
                    t.first(_index, t.second);
                    _tasks.pop();
                }

                {
                    std::lock_guard<std::mutex> lock(_mtx);
                    _condition.notify_one();
                }
            }
        }

        void Wait()
        {
            {
                std::unique_lock<std::mutex> lock(_mtx);
                _condition.wait(lock, [this]() -> bool { return _tasks.empty(); });
            }
        }

    private:
        friend class ThreadPool;

        uint _index;
        bool _destroyed;
        ThreadData* _data;
        std::thread _thread;
        std::mutex _mtx;
        std::condition_variable _condition;
        Queue<Pair<TaskCallback, void*>> _tasks;
    };

    ThreadPool::ThreadPool()
    {
        _threads.resize(std::thread::hardware_concurrency());

        ThreadData* pData = new ThreadData();
        _data = UniquePtr<ThreadData>(pData);

        for (uint i = 0; i < _threads.size(); i++)
        {
            _threads[i] = UniquePtr<WorkerThread>(new WorkerThread(i));
            _threads[i]->Create(pData);
        }
    }

    ThreadPool::~ThreadPool()
    {
        Wait();
        for (uint i = 0; i < _threads.size(); i++)
        {
            _threads[i]->Destroy();
        }
        _threads.clear();
    }

    ThreadPool& ThreadPool::Get()
    {
        static ThreadPool instance;
        return instance;
    }

    void ThreadPool::AddTask(TaskCallback callback, void* pData)
    {
        _data->tasks.push_back({ callback, pData });
    }

    void ThreadPool::Wait()
    {
        uint t = 0;
        for (uint i = 0; i < _data->tasks.size(); i++)
        {
            _threads[t]->_tasks.push(_data->tasks[i]);
            t = (t + 1) % _threads.size();
        }

        for (auto& thread : _threads)
            thread->_condition.notify_one();

        _data->tasks.clear();
        for (auto& thread : _threads)
            thread->Wait();
    }
}