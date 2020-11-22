#include <Windows.h>

#include "ISurface.h"
#include "CommandBuffer.h"
#include "Surface.h"

namespace SunEngine
{

	Surface::Surface() : GraphicsObject(GraphicsObject::SURFACE)
	{
		_iSurface = 0;
		_cmdBuffer = 0;
	}


	Surface::~Surface()
	{
	}

	bool Surface::Create(GraphicsWindow * pWindow)
	{
		if (!Destroy())
			return false;

		if (!_iSurface)
			_iSurface = AllocateGraphics<ISurface>();

		if (!_iSurface->Create(pWindow))
		{
			OutputDebugString("Failed Create Surface\n");
			return false;
		}

		_cmdBuffer = new CommandBuffer();
		_cmdBuffer->Create();

		if (!DerivedCreate())
		{
			OutputDebugString("Failed DerivedCreate Surface\n");
			return false;
		}

		return true;
	}

	bool Surface::Destroy()
	{
		if (!DerivedDestroy())
		{
			return false;
		}

		delete _cmdBuffer;
		_cmdBuffer = 0;

		if (!GraphicsObject::Destroy())
			return false;

		_iSurface = 0;
		return true;
	}

	IObject * Surface::GetAPIHandle() const
	{
		return _iSurface;
	}

	bool Surface::StartFrame()
	{
		return _iSurface->StartFrame(_cmdBuffer->GetAPIHandle());
	}

	bool Surface::EndFrame()
	{
		return _iSurface->SubmitFrame(_cmdBuffer->GetAPIHandle());
	}

	CommandBuffer* Surface::GetCommandBuffer() const
	{
		return _cmdBuffer;
	}

}