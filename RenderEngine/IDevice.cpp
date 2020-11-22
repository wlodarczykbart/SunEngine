#include "VulkanDevice.h"
#include "DirectXDevice.h"

#include "IDevice.h"

namespace SunEngine
{

	IDevice::IDevice()
	{
	}


	IDevice::~IDevice()
	{
	}

	IDevice * IDevice::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXDevice();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanDevice();
		default:
			return 0;
		}
	}

}