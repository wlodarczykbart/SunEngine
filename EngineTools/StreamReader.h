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

		bool Read(void *pBuffer, const usize size);

		bool Read(bool &buffer);
		bool Read(char &buffer);
		bool Read(uchar &buffer);
		bool Read(short &buffer);
		bool Read(ushort &buffer);
		bool Read(int &buffer);
		bool Read(uint &buffer);
		bool Read(float &buffer);
		bool Read(double &buffer);
		bool Read(long &buffer);
		bool Read(unsigned long &buffer);
		bool Read(String& buffer);

		template<typename T>
		bool Read(T*& buffer)
		{
			return Read(&buffer, sizeof(T*));
		}

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