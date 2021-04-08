#include "BufferBase.h"

namespace SunEngine
{

	BufferStream::BufferStream()
	{
		_pos = 0;
	}


	BufferStream::~BufferStream()
	{
	}

	void BufferStream::SetSize(uint size)
	{
		_data.resize(size);
		_pos = 0;
	}

	const uchar* BufferStream::GetData() const
	{
		return _data.data();
	}

	uchar* BufferStream::GetData()
	{
		return _data.data();
	}

	uint BufferStream::Tell() const
	{
		return _pos;
	}

	bool BufferStream::Seek(const uint offset, const Position pos)
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
			_pos = offset + _data.size();
			break;
		default:
			return false;
		}

		return _pos <= _data.size();
	}

	bool BufferStream::DerivedWrite(const void* pBuffer, const usize size)
	{
		if (_pos + size > _data.size())
			return false;

		memcpy(&_data[_pos], pBuffer, size);
		_pos += size;
		return true;
	}

	bool BufferStream::DerivedRead(void* pBuffer, const usize size)
	{
		if (_pos + size > _data.size())
			return false;

		memcpy(pBuffer, &_data[_pos], size);
		_pos += size;
		return true;
	}
}