#include "DirectXTextureArray.h"
#include "VulkanTextureArray.h" //TODO
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
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXTextureArray();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanTextureArray();
		default:
			return 0;
		}
	}

}