#include "VulkanGraphicsPipeline.h"
#include "D3D11GraphicsPipeline.h"
#include "IGraphicsPipeline.h"

namespace SunEngine
{

	IGraphicsPipeline::IGraphicsPipeline()
	{
	}


	IGraphicsPipeline::~IGraphicsPipeline()
	{
	}


	IGraphicsPipeline * IGraphicsPipeline::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanGraphicsPipeline();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11GraphicsPipeline();
		default:
			return 0;
		}
	}

}