#include "FileBase.h"


namespace SunEngine
{

	FileStream::FileStream()
	{
		_fh = 0;
	}


	FileStream::~FileStream()
	{
		Close();
	}

	bool FileStream::OpenForRead(const char* filename)
	{
		return fopen_s(&_fh, filename, "rb") == 0;
	}

	bool FileStream::OpenForWrite(const char* filename)
	{
		errno_t ferr = fopen_s(&_fh, filename, "wb");
		return ferr == 0;
	}

	uint FileStream::Tell() const
	{
		return (uint)ftell(_fh);
	}

	bool FileStream::Seek(const uint offset, const StreamBase::Position pos)
	{
		int fpos = -1;
		if (pos == StreamBase::Position::CURRENT)
			fpos = SEEK_CUR;
		if (pos == StreamBase::Position::START)
			fpos = SEEK_SET;
		if (pos == StreamBase::Position::END)
			fpos = SEEK_END;

		return fseek(_fh, (long)offset, fpos) == 0;
	}

	bool FileStream::DerivedWrite(const void* pBuffer, const usize size)
	{
		return fwrite(pBuffer, size, 1, _fh) > 0;
	}

	bool FileStream::DerivedRead(void* pBuffer, const usize size)
	{
		return fread(pBuffer, size, 1, _fh) > 0;
	}

	bool FileStream::Close()
	{
		if (_fh == 0)
			return true;

		bool closed = (fclose(_fh) == 0);
		_fh = 0;
		return closed;
	}

}