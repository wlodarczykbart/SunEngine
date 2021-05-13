#pragma once

#include "Types.h"
#include "Serializable.h"

namespace SunEngine
{
	class Resource : public Serializable
	{
	public:
		Resource();
		virtual ~Resource();
		Resource(const Resource&) = delete;
		Resource & operator = (const Resource&) = delete;

		const String& GetName() const { return _name; }

		virtual bool Write(StreamBase& stream) override;
		virtual bool Read(StreamBase& stream) override;

		void SetUserDataPtr(void* pData) { _userData = pData; }
		void* GetUserDataPtr() const { return _userData; }

	private:
		friend class ResourceMgr;

		String _name;
		void* _userData;
	};
}