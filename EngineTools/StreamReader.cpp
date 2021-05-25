#include "MemBuffer.h"

#include "StreamReader.h"


namespace SunEngine
{

	StreamReader::StreamReader()
	{
	}


	StreamReader::~StreamReader()
	{
	}

	bool StreamReader::Read(void *pBuffer, const usize size)
	{
		return DerivedRead(pBuffer, size);
	}

	bool StreamReader::Read(bool & buffer)
	{
		return Read(&buffer, sizeof(bool));
	}

	bool StreamReader::Read(char & buffer)
	{
		return Read(&buffer, sizeof(char));
	}

	bool StreamReader::Read(uchar & buffer)
	{
		return Read(&buffer, sizeof(uchar));
	}

	bool StreamReader::Read(short & buffer)
	{
		return Read(&buffer, sizeof(short));
	}

	bool StreamReader::Read(ushort & buffer)
	{
		return Read(&buffer, sizeof(ushort));
	}

	bool StreamReader::Read(int & buffer)
	{
		return Read(&buffer, sizeof(int));
	}

	bool StreamReader::Read(uint & buffer)
	{
		return Read(&buffer, sizeof(uint));
	}

	bool StreamReader::Read(float & buffer)
	{
		return Read(&buffer, sizeof(float));
	}

	bool StreamReader::Read(double & buffer)
	{
		return Read(&buffer, sizeof(double));
	}

	bool StreamReader::Read(long & buffer)
	{
		return Read(&buffer, sizeof(long));
	}

	bool StreamReader::Read(unsigned long & buffer)
	{
		return Read(&buffer, sizeof(unsigned long));
	}

	bool StreamReader::Read(uint64& buffer)
	{
		return Read(&buffer, sizeof(uint64));
	}

	bool StreamReader::Read(String & buffer)
	{
		uint len;
		if (!Read(len)) return false;
		if (len)
		{
			buffer.resize(len);
			if (!Read(&buffer[0], len)) return false;
		}
		else
		{
			buffer.clear();
		}
		return true;
	}
}
