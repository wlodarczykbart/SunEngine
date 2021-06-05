#include "VulkanDevice.h"
#define XR_USE_GRAPHICS_API_VULKAN
#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"


#include "VulkanVRInterface.h"

namespace SunEngine
{
	const char* VulkanVRInterface::GetExtensionName() const
	{
		return XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME;
	}

	bool VulkanVRInterface::Init(IVRInitInfo& info)
	{
		if (_device->_instance || _device->_device)
			return false;

		XrInstance instance = (XrInstance)info.inInstance;
		XrSystemId systemId = (XrSystemId)info.inSystemID;

		VkInstanceCreateInfo vkInstanceInfo = {};

		XrVulkanInstanceCreateInfoKHR instanceInfo = { XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
		instanceInfo.systemId = systemId;
		instanceInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
		return true;
	}

	bool VulkanVRInterface::InitTexture(VRHandle imgArray, uint imgIndex, int64 format, ITexture* pTexture)
	{
		return false;
	}

	void VulkanVRInterface::Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState)
	{
	}

	void VulkanVRInterface::Unbind(ICommandBuffer* cmdBuffer)
	{
	}
}