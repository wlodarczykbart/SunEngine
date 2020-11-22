#include "VulkanSampler.h"
#include "DirectXSampler.h"
#include "ISampler.h"

namespace SunEngine
{

	ISampler::ISampler()
	{
	}


	ISampler::~ISampler()
	{
	}

	ISampler * ISampler::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXSampler();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanSampler();
		default:
			return 0;
		}
	}

}