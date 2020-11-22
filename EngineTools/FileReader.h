#pragma once

#include "StreamReader.h"
#include "FileBase.h"

namespace SunEngine
{

	class FileReader : public StreamReader, public FileBase
	{
	public:
		FileReader();
		~FileReader();

		bool Open(const char * filename) override;

		uint Tell() const override;
		bool Seek(const uint offset, const Position pos) override;

	protected:
		bool DerivedRead(void *pBuffer, const usize size) override;
	};

}