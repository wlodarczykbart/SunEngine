#pragma once

#include "StreamBase.h"
#include <stdio.h>

namespace SunEngine
{

	class FileStream final : public StreamBase
	{
	public:
		FileStream();
		~FileStream();

		bool OpenForRead(const char* filename);
		bool OpenForWrite(const char* filename);
		bool Close();

	private:
		uint Tell() const override;
		bool Seek(const uint offset, const StreamBase::Position pos) override;
		bool DerivedWrite(const void* pBuffer, const usize size) override;
		bool DerivedRead(void* pBuffer, const usize size) override;

		FILE* _fh;
	};

}