#include "MemBuffer.h"


namespace SunEngine
{

	MemBuffer::MemBuffer()
	{
	}

	MemBuffer::MemBuffer(const uint size)
	{
		SetSize(size);
	}


	MemBuffer::~MemBuffer()
	{
	}

	void MemBuffer::SetSize(const uint size)
	{
		if(size != _buffer.size())
			_buffer.resize(size);
	}

	uint MemBuffer::GetSize() const
	{
		return  (uint)_buffer.size();
	}

	void MemBuffer::SetData(const void * pData)
	{
		memcpy(_buffer.data(), pData, _buffer.size());
	}

	void MemBuffer::SetData(const void * pData, const uint size, const uint offset)
	{
		if (size + offset <= _buffer.size())
		{
			usize memAddr = (usize)_buffer.data() + offset;
			memcpy((void*)memAddr, pData, size);
		}
	}

	char * MemBuffer::GetData()
	{
		return _buffer.data();
	}

	const char* MemBuffer::GetData() const
	{
		return _buffer.data();
	}

	char* MemBuffer::GetData(const uint offset)
	{
		return &_buffer[offset];
	}

	const char* MemBuffer::GetData(const uint offset) const
	{
		return &_buffer[offset];
	}

	bool MemBuffer::Write(StreamWriter &stream)
	{
		if (!stream.Write((uint)_buffer.size()))
			return false;

		if (_buffer.size())
		{
			if (!stream.Write(_buffer.data(), sizeof(char) * _buffer.size()))
				return false;
		}

		return true;
	}

	bool MemBuffer::Read(StreamReader &stream)
	{
		uint size;
		if (!stream.Read(size))
			return false;

		if (size)
		{
			this->SetSize(size);
			if (!stream.Read(_buffer.data(), sizeof(char) * _buffer.size()))
				return false;
		}

		return true;
	}

}