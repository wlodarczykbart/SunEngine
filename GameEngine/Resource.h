#pragma once

#include "Types.h"

namespace SunEngine
{
	class Resource
	{
	public:
		Resource();
		virtual ~Resource();
		Resource(const Resource&) = delete;
		Resource & operator = (const Resource&) = delete;

		const String& GetName() const { return _name; }

	private:
		friend class ResourceMgr;

		String _name;
	};
}