#include "VulkanDevice.h"
#include "D3D11Device.h"
#include "IDevice.h"

namespace SunEngine
{

	IDevice::IDevice()
	{
		_frameNumber = 0;
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

	void IDevice::SetFrameNumber(uint frameNumber)
	{
		_frameNumber = frameNumber;
	}

	uint IDevice::GetFrameNumber() const
	{
		return _frameNumber;
	}

}