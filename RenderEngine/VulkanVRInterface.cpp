#include "VulkanDevice.h"
#define XR_USE_GRAPHICS_API_VULKAN
#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"

#include "VulkanTexture.h"
#include "VulkanCommandBuffer.h"
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
		PFN_xrGetInstanceProcAddr GetInstanceProcAddr = (PFN_xrGetInstanceProcAddr)info.inGetInstanceProcAddr;

		XrGraphicsRequirementsVulkan2KHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };
		PFN_xrGetVulkanGraphicsRequirements2KHR GetVulkanGraphicsRequirements = 0;
		XR_RETURN_ON_FAIL(GetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)(&GetVulkanGraphicsRequirements)));
		XR_RETURN_ON_FAIL(GetVulkanGraphicsRequirements(instance, systemId, &graphicsRequirements));


		XrVulkanInstanceCreateInfoKHR instanceInfo = { XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
		instanceInfo.systemId = systemId;
		instanceInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
		//instanceInfo.vulkanCreateInfo = &vkInstanceInfo;
		instanceInfo.vulkanAllocator = 0;

		static PFN_xrCreateVulkanInstanceKHR CreateVulkanInstance = 0;
		XR_RETURN_ON_FAIL(GetInstanceProcAddr(instance, "xrCreateVulkanInstanceKHR", (PFN_xrVoidFunction*)(&CreateVulkanInstance)));

		auto createInstanceData = Pair<XrInstance, XrVulkanInstanceCreateInfoKHR*>(instance, &instanceInfo);
		if (!_device->createInstance(info.inDebugEnabled, [](const VkInstanceCreateInfo& info, VkInstance* pInstance, void* pData) -> VkResult {
			auto xrData = static_cast<Pair<XrInstance, XrVulkanInstanceCreateInfoKHR*>*>(pData);
			xrData->second->vulkanCreateInfo = &info;
			VkResult result;
			CreateVulkanInstance(xrData->first, xrData->second, pInstance, &result);
			return result;
		}, &createInstanceData))
			return false;

		if (!_device->createDebugCallback()) return false;

		XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo{ XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR };
		deviceGetInfo.systemId = systemId;
		deviceGetInfo.vulkanInstance = _device->_instance;

		PFN_xrGetVulkanGraphicsDevice2KHR GetVulkanGraphicsDevice = 0;
		XR_RETURN_ON_FAIL(GetInstanceProcAddr(instance, "xrGetVulkanGraphicsDevice2KHR", (PFN_xrVoidFunction*)(&GetVulkanGraphicsDevice)));
		XR_RETURN_ON_FAIL(GetVulkanGraphicsDevice(instance, &deviceGetInfo, &_device->_gpu));

		XrVulkanDeviceCreateInfoKHR deviceInfo{ XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR };
		deviceInfo.systemId = systemId;
		deviceInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
		//deviceCreateInfo.vulkanCreateInfo = &deviceInfo;
		//deviceCreateInfo.vulkanPhysicalDevice = m_vkPhysicalDevice;
		deviceInfo.vulkanAllocator = nullptr;

		static PFN_xrCreateVulkanDeviceKHR CreateVulkanDevice = 0;
		XR_RETURN_ON_FAIL(GetInstanceProcAddr(instance, "xrCreateVulkanDeviceKHR", (PFN_xrVoidFunction*)(&CreateVulkanDevice)));

		auto createDeviceData = Pair<XrInstance, XrVulkanDeviceCreateInfoKHR*>(instance, &deviceInfo);
		if (!_device->createDevice([](VkPhysicalDevice gpu, const VkDeviceCreateInfo& info, VkDevice* pDevice, void* pData) -> VkResult {
			auto xrData = static_cast<Pair<XrInstance, XrVulkanDeviceCreateInfoKHR*>*>(pData);
			xrData->second->vulkanCreateInfo = &info;
			xrData->second->vulkanPhysicalDevice = gpu;
			VkResult result;
			CreateVulkanDevice(xrData->first, xrData->second, pDevice, &result);
			return result;
		}, &createDeviceData))
			return false;

		if (!_device->createCommandPool()) return false;
		if (!_device->createDescriptorPool()) return false;
		if (!_device->allocCommandBuffers()) return false;

		static XrGraphicsBindingVulkan2KHR bindings = { XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR };
		bindings.instance = _device->_instance;
		bindings.physicalDevice = _device->_gpu;
		bindings.device = _device->_device;
		bindings.queueFamilyIndex = _device->_queue._familyIndex;
		bindings.queueIndex = 0;
		info.outBinding = &bindings;

		info.outSupportedSwapchainFormats =
		{
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_FORMAT_B8G8R8A8_SRGB,
		};

		static XrSwapchainImageVulkan2KHR imageHeaders[64];
		for (uint i = 0; i < ARRAYSIZE(imageHeaders); i++)
			imageHeaders[i].type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR;
		info.outImageHeaders = imageHeaders;

		return true;
	}

	bool VulkanVRInterface::InitTexture(VRHandle imgArray, uint imgIndex, int64 format, ITexture* pTexture)
	{
		VulkanTexture* pVulkanTexture = static_cast<VulkanTexture*>(pTexture);
		XrSwapchainImageVulkan2KHR& imgInfo = ((XrSwapchainImageVulkan2KHR*)imgArray)[imgIndex];

		pVulkanTexture->_image = imgInfo.image;
		pVulkanTexture->_memory = 0;
		pVulkanTexture->_sampleMask = VK_SAMPLE_COUNT_1_BIT;
		pVulkanTexture->_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkImageViewCreateInfo  viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = imgInfo.image;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		viewInfo.format = (VkFormat)format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.baseMipLevel = 0;// info.mipLevels ? 3 : 0;

		pVulkanTexture->_viewInfo = viewInfo;
		if (!_device->CreateImageView(pVulkanTexture->_viewInfo, &pVulkanTexture->_view)) return false;
		pVulkanTexture->_external = true;

		return true;
	}

	void VulkanVRInterface::Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState)
	{
		cmdBuffer->Begin();
	}

	void VulkanVRInterface::Unbind(ICommandBuffer* cmdBuffer)
	{
		//TODO: Poor way to do this as Submit() waits on fences for command buffer completion
		cmdBuffer->End();
		cmdBuffer->Submit();
	}
}