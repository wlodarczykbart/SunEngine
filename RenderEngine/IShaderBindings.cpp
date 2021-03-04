#include "VulkanShader.h"
#include "D3D11Shader.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanShaderBindings();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11ShaderBindings();
		default:
			return 0;
		}
	}

}