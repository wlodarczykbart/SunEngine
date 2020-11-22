#pragma once

#include "StreamBase.h"

namespace SunEngine
{

	class StreamWriter : public StreamBase
	{
	public:
		StreamWriter();
		virtual ~StreamWriter();

		bool Write(const void *pBuffer, const usize size);

		bool Write(const bool buffer);
		bool Write(const char buffer);
		bool Write(const uchar buffer);
		bool Write(const short buffer);
		bool Write(const ushort buffer);
		bool Write(const int buffer);
		bool Write(const uint buffer);
		bool Write(const float buffer);
		bool Write(const double buffer);
		bool Write(const long buffer);
		bool Write(const unsigned long buffer);
		bool Write(const String& buffer);
		bool Write(const char* pBuffer);
		bool Write(void* const* pBuffer);

	protected:
		virtual bool DerivedWrite(const void *pBuffer, const usize size) = 0;
	};

}