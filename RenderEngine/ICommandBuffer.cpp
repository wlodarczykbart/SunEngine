#include "DirectXCommandBuffer.h"
#include "VulkanCommandBuffer.h"
#include "ICommandBuffer.h"

namespace SunEngine
{

	ICommandBuffer::ICommandBuffer()
	{
	}


	ICommandBuffer::~ICommandBuffer()
	{
	}

	ICommandBuffer * ICommandBuffer::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXCommandBuffer();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanCommandBuffer();
		default:
			return 0;
		}
	}

}