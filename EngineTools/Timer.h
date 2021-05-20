#pragma once

namespace SunEngine
{
	class Timer final
	{
	public:
		explicit Timer(bool startOnConstructor = false);
		~Timer();

		void Start();
		void Stop();

		double Tick();
		double ElapsedTime();

	private:
		double mOneOverFrequency;
		double mElapsedTime;
		long long mCounter;
	};
}