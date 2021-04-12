#pragma once

#include "GraphicsObject.h"

namespace SunEngine
{
	class GraphicsWindow;
	class CommandBuffer;

	class Surface : public GraphicsObject
	{
	public:
		Surface();
		virtual ~Surface();

		bool Create(GraphicsWindow* pWindow);
		bool Destroy();

		IObject* GetAPIHandle() const override;

		bool StartFrame();
		bool EndFrame();

		CommandBuffer* GetCommandBuffer() const;

	protected:
		virtual bool DerivedCreate() { return true; }
		virtual bool DerivedDestroy() { return true; }

		ISurface* _iSurface;
		Vector<CommandBuffer*> _cmdBuffers;
		uint _frameIndex;
	};

}