#include "FileReader.h"



namespace SunEngine
{

	FileReader::FileReader()
	{
	}


	FileReader::~FileReader()
	{
	}

	bool FileReader::Open(const char * filename)
	{
		return fopen_s(&_fh, filename, "rb") == 0;
	}

	bool FileReader::DerivedRead(void * pBuffer, const usize size)
	{
		return fread(pBuffer, size, 1, _fh) > 0;
	}

	uint FileReader::Tell() const
	{
		return FileBase::Tell();
	}

	bool FileReader::Seek(const uint offset, const Position pos)
	{
		return FileBase::Seek(offset, pos);
	}
}