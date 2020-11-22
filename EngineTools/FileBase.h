#pragma once

#include "StreamBase.h"
#include <stdio.h>

namespace SunEngine
{

	class FileBase
	{
	public:
		FileBase();
		virtual ~FileBase();

		virtual bool Open(const char * filename) = 0;
		bool Close();

	protected:
		uint Tell() const;
		bool Seek(const uint offset, const StreamBase::Position pos);

		FILE* _fh;
	};

}