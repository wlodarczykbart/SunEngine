#include "VulkanDevice.h"
#include "D3D11Device.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanDevice();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11Device();
		default:
			return 0;
		}
	}

}