#pragma once

#include "Types.h"

namespace SunEngine
{
	class MemBuffer;

	class StreamReader
	{
	public:
		StreamReader();
		virtual ~StreamReader();

		virtual bool Read(void *pBuffer, const usize size);

		virtual bool Read(bool &buffer);
		virtual bool Read(char &buffer);
		virtual bool Read(uchar &buffer);
		virtual bool Read(short &buffer);
		virtual bool Read(ushort &buffer);
		virtual bool Read(int &buffer);
		virtual bool Read(uint &buffer);
		virtual bool Read(float &buffer);
		virtual bool Read(double &buffer);
		virtual bool Read(long &buffer);
		virtual bool Read(unsigned long &buffer);
		virtual bool Read(String& buffer);
		virtual bool Read(void** buffer);

		template<typename K, typename V>
		bool ReadSimple( Map<K, V>& map)
		{
			uint size = 0;
			if (!Read(size))
				return false;

			for (uint i = 0; i < size; i++)
			{
				K key;
				if (!Read(key))
					return false;

				V value;
				if (!Read(&value, sizeof(V)))
					return false;

				map[key] = value;
			}

			return true;
		}

	protected:
		virtual bool DerivedRead(void *pBuffer, const usize size) = 0;
	};

}