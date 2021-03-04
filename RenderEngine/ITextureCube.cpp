#include "VulkanTextureCube.h"
#include "D3D11TextureCube.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanTextureCube();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11TextureCube();
		default:
			return 0;
		}
	}

}