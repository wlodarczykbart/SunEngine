#include "DirectXTextureCube.h"
#include "VulkanTextureCube.h"
#include "ITextureCube.h"


namespace SunEngine
{
	ITextureCube::ITextureCube()
	{
	}


	ITextureCube::~ITextureCube()
	{
	}

	ITextureCube * ITextureCube::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXTextureCube();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanTextureCube();
		default:
			return 0;
		}
	}

}