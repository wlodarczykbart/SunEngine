#include "TimeImpl.h"

namespace SunEngine
{
	float Time::_delta = 0.0f;
	float Time::_elapsed = 0.0f;


	Time::Time()
	{
	}


	Time::~Time()
	{
	}

	float Time::Delta()
	{
		return _delta;
	}

	float Time::Elapsed()
	{
		return _elapsed;
	}

}