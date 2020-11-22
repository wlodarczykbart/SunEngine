#pragma once

#include "StreamReader.h"
#include "StreamWriter.h"

namespace SunEngine
{

	class Serializable
	{
	public:
		Serializable();
		virtual ~Serializable();

		virtual bool Write(StreamWriter &stream) = 0;
		virtual bool Read(StreamReader &stream) = 0;

	private:
	};

}

