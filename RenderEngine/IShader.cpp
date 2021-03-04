#include "VulkanShader.h"
#include "D3D11Shader.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanShader();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11Shader();
		default:
			return 0;
		}
	}

}