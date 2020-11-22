#include "StreamWriter.h"



namespace SunEngine
{

	StreamWriter::StreamWriter()
	{
	}


	StreamWriter::~StreamWriter()
	{
	}

	bool StreamWriter::Write(const void * pBuffer, const usize size)
	{
		return DerivedWrite(pBuffer, size);
	}

	bool StreamWriter::Write(const bool buffer)
	{
		return Write(&buffer, sizeof(bool));
	}

	bool StreamWriter::Write(const char buffer)
	{
		return Write(&buffer, sizeof(char));
	}

	bool StreamWriter::Write(const uchar buffer)
	{
		return Write(&buffer, sizeof(uchar));
	}

	bool StreamWriter::Write(const short buffer)
	{
		return Write(&buffer, sizeof(short));
	}

	bool StreamWriter::Write(const ushort buffer)
	{
		return Write(&buffer, sizeof(ushort));
	}

	bool StreamWriter::Write(const int buffer)
	{
		return Write(&buffer, sizeof(int));
	}

	bool StreamWriter::Write(const uint buffer)
	{
		return Write(&buffer, sizeof(uint));
	}

	bool StreamWriter::Write(const float buffer)
	{
		return Write(&buffer, sizeof(float));
	}

	bool StreamWriter::Write(const double buffer)
	{
		return Write(&buffer, sizeof(double));
	}

	bool StreamWriter::Write(const long buffer)
	{
		return Write(&buffer, sizeof(long));
	}

	bool StreamWriter::Write(const unsigned long buffer)
	{
		return Write(&buffer, sizeof(unsigned long));
	}

	bool StreamWriter::Write(const String & buffer)
	{
		if (!Write((uint)buffer.length())) return false;
		if (buffer.length())
		{
			if (!Write(buffer.data(), buffer.length()))
				return false;
		}

		return true;
	}

	bool StreamWriter::Write(const char * pBuffer)
	{
		if (!Write(pBuffer, strlen(pBuffer))) return false;
		return true;
	}

	bool StreamWriter::Write(void* const* pBuffer)
	{
		return Write(pBuffer, sizeof(void*));
	}

}