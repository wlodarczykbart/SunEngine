#pragma once

#include "IObject.h"

namespace SunEngine
{
	class GraphicsWindow;

	class ISurface : public IObject
	{
	public:
		ISurface();
		virtual ~ISurface();

		virtual bool Create(GraphicsWindow *window) = 0;
		virtual bool StartFrame(ICommandBuffer* cmdBuffer) = 0;
		virtual bool SubmitFrame(ICommandBuffer* cmdBuffer) = 0;

		static ISurface* Allocate(GraphicsAPI api);
	};
}