#include "DirectXShader.h"
#include "VulkanShader.h"

#include "IShaderBindings.h"

namespace SunEngine
{

	IShaderBindings::IShaderBindings()
	{
	}


	IShaderBindings::~IShaderBindings()
	{
	}

	IShaderBindings * IShaderBindings::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXShaderBindings();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanShaderBindings();
		default:
			return 0;
		}
	}

}