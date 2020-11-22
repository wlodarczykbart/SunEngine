#include "VulkanRenderTarget.h"
#include "DirectXRenderTarget.h"

#include "IRenderTarget.h"

namespace SunEngine
{
	IRenderTarget::IRenderTarget()
	{
	}


	IRenderTarget::~IRenderTarget()
	{
	}

	IRenderTarget * IRenderTarget::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXRenderTarget();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanRenderTarget();
		default:
			return 0;
		}
	}
}