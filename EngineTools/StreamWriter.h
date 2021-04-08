#pragma once

#include "Types.h"

namespace SunEngine
{

	class StreamWriter
	{
	public:
		StreamWriter();
		virtual ~StreamWriter();

		bool Write(const void *pBuffer, const usize size);

		bool Write(const bool buffer);
		bool Write(const char buffer);
		bool Write(const uchar buffer);
		bool Write(const short buffer);
		bool Write(const ushort buffer);
		bool Write(const int buffer);
		bool Write(const uint buffer);
		bool Write(const float buffer);
		bool Write(const double buffer);
		bool Write(const long buffer);
		bool Write(const unsigned long buffer);
		bool Write(const String& buffer);
		bool Write(const void* pBuffer);
		
		template<typename K, typename V>
		bool WriteSimple(const Map<K, V>& map)
		{
			uint size = map.size();
			if (!Write(size))
				return false;

			for (auto iter = map.begin(); iter != map.end(); ++iter)
			{
				if (!Write((*iter).first))
					return false;

				if(!Write(&(*iter).second, sizeof(V)))
					return false;
			}

			return true;
		}

		template<typename T>
		bool WriteSimple(Vector<T>& vec)
		{
			uint size = (uint)vec.size();
			if (!Write(size))
				return false;

			if (size)
			{
				if (!Write(vec.data(), sizeof(T) * size))
					return false;
			}

			return true;
		}

	protected:
		virtual bool DerivedWrite(const void *pBuffer, const usize size) = 0;
	};
}