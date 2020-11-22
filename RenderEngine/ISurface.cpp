#include "VulkanSurface.h"
#include "DirectXSurface.h"

#include "ISurface.h"

namespace SunEngine
{

	ISurface::ISurface()
	{
	}


	ISurface::~ISurface()
	{
	}

	ISurface * ISurface::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXSurface();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanSurface();
		default:
			return 0;
		}
	}

}