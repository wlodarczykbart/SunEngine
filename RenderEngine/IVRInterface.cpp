#include "VulkanVRInterface.h"
#include "D3D11VRInterface.h"
#include "IVRInterface.h"

namespace SunEngine
{

	IVRInterface::IVRInterface()
	{
	}


	IVRInterface::~IVRInterface()
	{
	}

	IVRInterface* IVRInterface::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanVRInterface();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11VRInterface();
		default:
			return 0;
		}
	}

}