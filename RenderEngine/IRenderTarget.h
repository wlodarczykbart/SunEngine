#pragma once

#include "IObject.h"

namespace SunEngine
{
	class IRenderTarget : public IObject
	{
	public:
		IRenderTarget();
		virtual ~IRenderTarget();

		virtual bool Create(const IRenderTargetCreateInfo &info) = 0;
		static IRenderTarget * Allocate(GraphicsAPI api);
	};
}