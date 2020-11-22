#pragma once

#include "StreamBase.h"

namespace SunEngine
{
	class MemBuffer;

	class StreamReader : public StreamBase
	{
	public:
		StreamReader();
		virtual ~StreamReader();

		virtual bool Read(void *pBuffer, const usize size);

		virtual bool Read(bool &buffer);
		virtual bool Read(char &buffer);
		virtual bool Read(uchar &buffer);
		virtual bool Read(short &buffer);
		virtual bool Read(ushort &buffer);
		virtual bool Read(int &buffer);
		virtual bool Read(uint &buffer);
		virtual bool Read(float &buffer);
		virtual bool Read(double &buffer);
		virtual bool Read(long &buffer);
		virtual bool Read(unsigned long &buffer);
		virtual bool Read(String& buffer);
		virtual bool Read(void** buffer);

		virtual bool ReadAll(MemBuffer &buffer);
		virtual bool ReadAllText(String &buffer);

	protected:
		virtual bool DerivedRead(void *pBuffer, const usize size) = 0;
	};

}