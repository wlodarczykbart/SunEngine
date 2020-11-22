#include <Windows.h>

#include "Timer.h"


namespace SunEngine
{

	Timer::Timer()
	{



	}


	Timer::~Timer()
	{
	}

	void Timer::Start()
	{
		LARGE_INTEGER frequency, counter;
		QueryPerformanceFrequency(&frequency);
		this->mOneOverFrequency = 1.0 / (double)frequency.QuadPart;
		QueryPerformanceCounter(&counter);
		this->mCounter = counter.QuadPart;

		this->mElapsedTime = 0.0;
	}

	void Timer::Stop()
	{
		Tick();
		this->mOneOverFrequency = 0.0;
		this->mCounter = 0;
	}

	double Timer::Tick()
	{
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);

		long long interval = counter.QuadPart - this->mCounter;
		double deltaTime = (double)interval * mOneOverFrequency;

		mElapsedTime += deltaTime;

		this->mCounter = counter.QuadPart;

		return deltaTime;
	}

	double Timer::ElapsedTime()
	{
		return mElapsedTime;
	}

}