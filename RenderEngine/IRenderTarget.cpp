#include "VulkanRenderTarget.h"
#include "D3D11RenderTarget.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanRenderTarget();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11RenderTarget();
		default:
			return 0;
		}
	}
}