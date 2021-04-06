#include "MemBuffer.h"
#include "StreamBase.h"

namespace SunEngine
{

	StreamBase::StreamBase()
	{
	}


	StreamBase::~StreamBase()
	{
	}

	uint StreamBase::Size()
	{
		uint curr = Tell();
		if (Seek(0, START))
		{
			Seek(0, END);
			uint size = Tell();
			Seek(curr, START);
			return size;
		}
		else
		{
			return 0;
		}
	}

	bool StreamBase::ReadBuffer(MemBuffer& buffer)
	{
		buffer.SetSize(Size());

		Seek(0, START);
		bool bRead = Read(buffer.GetData(), buffer.GetSize());
		if (bRead)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool StreamBase::ReadText(String& buffer)
	{
		buffer.resize(Size());

		Seek(0, START);
		bool bRead = Read(&buffer[0], buffer.size());
		if (bRead)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool StreamBase::WriteText(const String& buffer)
	{
		return Write(buffer.data(), buffer.length());
	}

	bool StreamBase::SeekStart()
	{
		return Seek(0, Position::START);
	}

	NullStream::NullStream()
	{
		_pos = 0;
		_size = 0;
	}

	uint NullStream::Tell() const
	{
		return _pos;
	}

	bool NullStream::Seek(const uint offset, const Position pos)
	{
		switch (pos)
		{
		case Position::CURRENT:
			_pos += offset;
			break;
		case Position::START:
			_pos = offset;
			break;
		case Position::END:
			_pos = _size + offset;
			break;
		default:
			break;
		}

		return true;
	}

	bool NullStream::DerivedWrite(const void*, const usize size)
	{
		_pos += size;
		if (_pos > size)
			_size = _pos;

		return true;
	}

	bool NullStream::DerivedRead(void*, const usize size)
	{
		if (_pos + size > _size)
			return false;

		_pos += size;
		return true;
	}
}