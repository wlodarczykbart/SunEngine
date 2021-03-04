#include "VulkanSurface.h"
#include "D3D11Surface.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanSurface();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11Surface();
		default:
			return 0;
		}
	}

}