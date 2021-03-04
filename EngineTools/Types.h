#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <map>
#include <vector>
#include <list>
#include <queue>
#include <array>
#include <stack>
#include <unordered_set>

#define SE_ARR_SIZE(a) (sizeof(a) / sizeof(*a))

namespace SunEngine
{
	typedef unsigned int uint;
	typedef unsigned short ushort;
	typedef unsigned char uchar;
	typedef size_t usize;

	typedef std::string String;

	template<typename T>
	using Vector = std::vector<T>;

	template<typename T>
	using LinkedList = std::list<T>;

	template<typename T>
	using Queue = std::queue<T>;

	template<typename K, typename V, typename Hash = std::hash<K>>
	using Map = std::unordered_map<K, V, Hash>;

	template<typename V>
	using StrMap = Map<String, V>;

	template<typename K, typename V>
	using OrderedMap = std::map<K, V>;

	template<typename V>
	using OrderedStrMap = OrderedMap<String, V>;

	template<typename T, usize size = 0>
	using Array = std::array<T, size>;

	template<typename T>
	using Stack = std::stack<T>;

	template<typename T>
	using HashSet = std::unordered_set<T>;

	template<typename T1, typename T2>
	using Pair = std::pair<T1, T2>;

	template<typename T>
	using UniquePtr = std::unique_ptr<T>;

	template<typename T>
	bool Contains(const Vector<T>& vec, const T& val)
	{
		return std::find(vec.begin(), vec.end(), val) != vec.end();
	}

	template<typename T>
	T* Find(Vector<T> &vec, const void* pUserData, bool(*CompareFunc)(const T& val, const void* userData))
	{
		for (uint i = 0; i < vec.size(); i++)
		{
			if (CompareFunc(vec[i], pUserData))
				return &vec[i];
		}

		return 0;
	}

	//template<typename T>
	//UniquePtr<T> MakeUnique()
	//{
	//	std::make_unique<T>();
	//}

}