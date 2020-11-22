#pragma once

#include "GraphicsAPIDef.h"

namespace SunEngine
{
	class ICommandBuffer;

	class IObject
	{
	public:
		virtual ~IObject();

		virtual void Bind(ICommandBuffer *cmdBuffer) = 0;
		virtual void Unbind(ICommandBuffer* cmdBuffer) = 0;

		virtual bool Destroy() { return true; }; //todo make this pure virtual...

	protected:
		IObject();
	};

}
