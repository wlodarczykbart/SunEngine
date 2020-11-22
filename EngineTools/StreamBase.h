#pragma once

#include "Types.h"

namespace SunEngine
{
	class StreamBase
	{
	public:
		enum Position
		{
			START,
			CURRENT,
			END,
		};

		StreamBase();
		virtual ~StreamBase();

		virtual uint Tell() const = 0;
		virtual bool Seek(const uint offset, const Position pos) = 0;
		uint Size();
	};
}