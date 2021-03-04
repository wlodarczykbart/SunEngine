#include "VulkanTextureArray.h" 
#include "D3D11TextureArray.h"
#include "ITextureArray.h"


namespace SunEngine
{
	ITextureArray::ITextureArray()
	{
	}


	ITextureArray::~ITextureArray()
	{
	}

	ITextureArray * ITextureArray::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanTextureArray();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11TextureArray();
		default:
			return 0;
		}
	}

}