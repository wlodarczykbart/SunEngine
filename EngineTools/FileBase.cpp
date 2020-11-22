#include "FileBase.h"


namespace SunEngine
{

	FileBase::FileBase()
	{
		_fh = 0;
	}


	FileBase::~FileBase()
	{
		Close();
	}

	uint FileBase::Tell() const
	{
		return (uint)ftell(_fh);
	}

	bool FileBase::Seek(const uint offset, const StreamBase::Position pos)
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

	bool FileBase::Close()
	{
		if (_fh == 0)
			return true;

		bool closed = (fclose(_fh) == 0);
		_fh = 0;
		return closed;
	}

}