#include "VulkanSampler.h"
#include "D3D11Sampler.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanSampler();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11Sampler();
		default:
			return 0;
		}
	}

}