#include "VulkanTexture.h"
#include "DirectXTexture.h"

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
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXTexture();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanTexture();
		default:
			return 0;
		}
	}

}