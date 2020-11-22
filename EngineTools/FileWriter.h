#pragma once

#include "StreamWriter.h"
#include "FileBase.h"

namespace SunEngine
{

	class FileWriter : public StreamWriter, public FileBase
	{
	public:
		FileWriter();
		~FileWriter();

		bool Open(const char * fileName) override;

		uint Tell() const override;
		bool Seek(const uint offset, const Position pos) override;

	private:
		bool DerivedWrite(const void *pBuffer, const usize size) override;
	};

}
