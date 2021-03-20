#pragma once

#include "StreamBase.h"

namespace SunEngine
{
	class BufferStream final : public StreamBase
	{
	public:
		BufferStream();
		~BufferStream();

		void SetSize(uint size);

	private:
		uint Tell() const override;
		bool Seek(const uint offset, const Position pos) override;
		bool DerivedWrite(const void* pBuffer, const usize size) override;
		bool DerivedRead(void* pBuffer, const usize size) override;

		uint _pos;
		Vector<uchar> _data;
	};
}
