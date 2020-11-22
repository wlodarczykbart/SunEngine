#include "VulkanGraphicsPipeline.h"
#include "DirectXGraphicsPipeline.h"

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
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXGraphicsPipeline();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanGraphicsPipeline();
		default:
			return 0;
		}
	}

}