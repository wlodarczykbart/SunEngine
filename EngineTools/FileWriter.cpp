#include "FileWriter.h"



namespace SunEngine
{

	FileWriter::FileWriter()
	{
	}


	FileWriter::~FileWriter()
	{
	}

	bool FileWriter::Open(const char * fileName)
	{
		return fopen_s(&_fh, fileName, "wb") == 0;
	}

	bool FileWriter::DerivedWrite(const void * pBuffer, const usize size)
	{
		return fwrite(pBuffer, size, 1, _fh) > 0;
	}

	uint FileWriter::Tell() const
	{
		return FileBase::Tell();
	}

	bool FileWriter::Seek(const uint offset, const Position pos)
	{
		return FileBase::Seek(offset, pos);
	}

}
