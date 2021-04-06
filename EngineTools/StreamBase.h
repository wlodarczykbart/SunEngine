#pragma once

#include "Types.h"
#include "StreamReader.h"
#include "StreamWriter.h"


namespace SunEngine
{
	class MemBuffer;

	class StreamBase : public StreamWriter, public StreamReader
	{
	public:
		enum Position
		{
			START,
			CURRENT,
			END,
		};

		StreamBase();
		virtual ~StreamBase();

		virtual uint Tell() const = 0;
		virtual bool Seek(const uint offset, const Position pos) = 0;
		uint Size();

		bool ReadBuffer(MemBuffer& buffer);
		bool ReadText(String& buffer);
		bool WriteText(const String& buffer);

		bool SeekStart();
	};

	//Useful to determining the size of a object that can be streamed
	class NullStream final : public StreamBase
	{
	public:
		NullStream();

		uint Tell() const;
		bool Seek(const uint offset, const Position pos);
	private:
		bool DerivedWrite(const void* pBuffer, const usize size) override;
		bool DerivedRead(void* pBuffer, const usize size) override;
		uint _pos;
		uint _size;
	};
}