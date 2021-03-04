#include "VulkanCommandBuffer.h"
#include "D3D11CommandBuffer.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanCommandBuffer();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11CommandBuffer();
		default:
			return 0;
		}
	}

}