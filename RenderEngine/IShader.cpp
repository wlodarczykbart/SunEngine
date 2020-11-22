#include "VulkanShader.h"
#include "DirectXShader.h"

#include "IShader.h"

namespace SunEngine
{
	IShader::IShader()
	{
	}


	IShader::~IShader()
	{
	}

	IShader * IShader::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXShader();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanShader();
		default:
			return 0;
		}
	}

}