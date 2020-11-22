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
		virtual void SetClearColor(const float r, const float g, const float b, const float a) = 0;
		virtual void SetClearOnBind(bool clear) = 0;
		virtual void SetViewport(float x, float y, float width, float height) = 0;

		static IRenderTarget * Allocate(GraphicsAPI api);
	};
}