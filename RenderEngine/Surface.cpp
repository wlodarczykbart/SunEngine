#include <Windows.h>

#include "ISurface.h"
#include "CommandBuffer.h"
#include "Surface.h"

namespace SunEngine
{

	Surface::Surface() : GraphicsObject(GraphicsObject::SURFACE)
	{
		_iSurface = 0;
		_frameIndex = 0;
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

		_cmdBuffers.resize(_iSurface->GetBackBufferCount());
		for (uint i = 0; i < _cmdBuffers.size(); i++)
		{
			_cmdBuffers[i] = new CommandBuffer();
			_cmdBuffers[i]->Create();
		}

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

		for (uint i = 0; i < _cmdBuffers.size(); i++)
		{
			delete _cmdBuffers[i];
		}
		_cmdBuffers.clear();

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
		return _iSurface->StartFrame(_cmdBuffers[_frameIndex]->GetAPIHandle());
	}

	bool Surface::EndFrame()
	{
		bool status = _iSurface->SubmitFrame(_cmdBuffers[_frameIndex]->GetAPIHandle());
		_frameIndex++;
		_frameIndex %= _cmdBuffers.size();
		return status;
	}

	CommandBuffer* Surface::GetCommandBuffer() const
	{
		return _cmdBuffers[_frameIndex];
	}

	uint Surface::GetBackBufferCount() const
	{
		return _iSurface->GetBackBufferCount();
	}

	uint Surface::GetFrameIndex() const
	{
		return _frameIndex;
	}

}