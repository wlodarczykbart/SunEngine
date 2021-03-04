#include "VulkanTexture.h"
#include "D3D11Texture.h"
#include "ITexture.h"

namespace SunEngine
{

	ITexture::ITexture()
	{
	}


	ITexture::~ITexture()
	{
	}

	ITexture * ITexture::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanTexture();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11Texture();
		default:
			return 0;
		}
	}

}