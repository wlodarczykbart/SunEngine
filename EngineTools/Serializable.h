#pragma once

#include "StreamBase.h"

namespace SunEngine
{

	class Serializable
	{
	public:
		Serializable();
		virtual ~Serializable();

		virtual bool Write(StreamBase &stream) = 0;
		virtual bool Read(StreamBase &stream) = 0;

	private:
	};

}

