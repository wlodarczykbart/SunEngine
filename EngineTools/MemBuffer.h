#pragma once

#include "Serializable.h"


namespace SunEngine
{
	class MemBuffer : public Serializable
	{
	public:
		MemBuffer();
		MemBuffer(const uint size);
		~MemBuffer();

		void SetSize(const uint size);
		uint GetSize() const;

		void SetData(const void* pData);
		void SetData(const void *pData, const uint size, const uint offset = 0);

		char* GetData();
		const char* GetData() const;
		char* GetData(const uint offset);
		const char* GetData(const uint offset) const;

		bool Write(StreamWriter &stream) override;
		bool Read(StreamReader &stream) override;

	private:
		Vector<char> _buffer;
	};

}
