#include "CommandBuffer.h"
#include "GraphicsAPIDef.h"
#include "VulkanObject.h"

#include "IObject.h"
#include "GraphicsObject.h"


namespace SunEngine
{

	GraphicsObject::GraphicsObject(GraphicsObject::Type type)
	{
		_type = type;
	}


	GraphicsObject::~GraphicsObject()
	{
	}

	const String& GraphicsObject::GetErrStr() const
	{
		return _errStr;
	}

	GraphicsObject::Type GraphicsObject::GetType() const
	{
		return _type;
	}

	bool GraphicsObject::Bind(CommandBuffer* cmdBuffer, IBindState* pBindState)
	{
		IObject* pObj = this->GetAPIHandle();
		if (!pObj)
		{
			_errStr = "IObject is null pointer.";
			return false;
		}

		pObj->Bind(cmdBuffer->GetAPIHandle(), pBindState);

		return true;
	}

	bool GraphicsObject::Unbind(CommandBuffer* cmdBuffer)
	{
		IObject* pObj = this->GetAPIHandle();
		if (!pObj)
		{
			_errStr = "IObject is null pointer.";
			return false;
		}

		pObj->Unbind(cmdBuffer->GetAPIHandle());

		return true;
	}

	bool GraphicsObject::Destroy()
	{
		IObject* obj = this->GetAPIHandle();
		if (obj)
		{
			if (!obj->Destroy())
				return false;

			delete obj;
			return true;
		}
		else
		{
			return true;
		}
	}
}