#pragma once

namespace SunEngine
{
	class Time final
	{
	public:

		static float Delta();
		static float Elapsed();

	private:
		friend class GraphicsApp;
		Time();
		~Time();

		static float _delta;
		static float _elapsed;
	};

}