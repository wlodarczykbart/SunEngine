#pragma once

#include "Types.h"

namespace SunEngine
{
	class ThreadPool
	{
	public:
		typedef void(*TaskCallback)(uint threadIndex, void* pData);

		static ThreadPool& Get();

		void AddTask(TaskCallback callback, void* pData);
		void Wait();

		uint GetThreadCount() const { return _threads.size(); }

	private:
		class WorkerThread;
		struct ThreadData;

		ThreadPool();
		ThreadPool(const ThreadPool&) = delete;
		ThreadPool& operator= (const ThreadPool&) = delete;
		~ThreadPool();

		UniquePtr<ThreadData> _data;
		Vector<UniquePtr<WorkerThread>> _threads;
	};
}