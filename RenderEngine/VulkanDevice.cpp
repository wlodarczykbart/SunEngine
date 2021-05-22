#include <assert.h>
#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include "StringUtil.h"

//Copied from Vulkan.h, keep updated?

#define CheckVkResult(expression) if(expression != VK_SUCCESS) { auto strErr = VulkanResultStrings.find(expression); _errLine = StrFormat("err: %s, line: %d", strErr != VulkanResultStrings.end() ? (*strErr).second.data() : std::to_string(expression).data(), __LINE__); return false; }

namespace SunEngine
{
	const Map<VkResult, String> VulkanResultStrings = {
		{ VK_SUCCESS , "VK_SUCCESS" },
		{ VK_NOT_READY , "VK_NOT_READY" },
		{ VK_TIMEOUT , "VK_TIMEOUT" },
		{ VK_EVENT_SET , "VK_EVENT_SET" },
		{ VK_EVENT_RESET , "VK_EVENT_RESET" },
		{ VK_INCOMPLETE , "VK_INCOMPLETE" },
		{ VK_ERROR_OUT_OF_HOST_MEMORY , "VK_ERROR_OUT_OF_HOST_MEMORY" },
		{ VK_ERROR_OUT_OF_DEVICE_MEMORY , "VK_ERROR_OUT_OF_DEVICE_MEMORY" },
		{ VK_ERROR_INITIALIZATION_FAILED , "VK_ERROR_INITIALIZATION_FAILED" },
		{ VK_ERROR_DEVICE_LOST , "VK_ERROR_DEVICE_LOST" },
		{ VK_ERROR_MEMORY_MAP_FAILED , "VK_ERROR_MEMORY_MAP_FAILED" },
		{ VK_ERROR_LAYER_NOT_PRESENT , "VK_ERROR_LAYER_NOT_PRESENT" },
		{ VK_ERROR_EXTENSION_NOT_PRESENT , "VK_ERROR_EXTENSION_NOT_PRESENT" },
		{ VK_ERROR_FEATURE_NOT_PRESENT , "VK_ERROR_FEATURE_NOT_PRESENT" },
		{ VK_ERROR_INCOMPATIBLE_DRIVER , "VK_ERROR_INCOMPATIBLE_DRIVER" },
		{ VK_ERROR_TOO_MANY_OBJECTS , "VK_ERROR_TOO_MANY_OBJECTS" },
		{ VK_ERROR_FORMAT_NOT_SUPPORTED , "VK_ERROR_FORMAT_NOT_SUPPORTED" },
		{ VK_ERROR_FRAGMENTED_POOL , "VK_ERROR_FRAGMENTED_POOL" },
		{ VK_ERROR_OUT_OF_POOL_MEMORY , "VK_ERROR_OUT_OF_POOL_MEMORY" },
		{ VK_ERROR_INVALID_EXTERNAL_HANDLE , "VK_ERROR_INVALID_EXTERNAL_HANDLE" },
		{ VK_ERROR_SURFACE_LOST_KHR , "VK_ERROR_SURFACE_LOST_KHR" },
		{ VK_ERROR_NATIVE_WINDOW_IN_USE_KHR , "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" },
		{ VK_SUBOPTIMAL_KHR , "VK_SUBOPTIMAL_KHR" },
		{ VK_ERROR_OUT_OF_DATE_KHR , "VK_ERROR_OUT_OF_DATE_KHR" },
		{ VK_ERROR_INCOMPATIBLE_DISPLAY_KHR , "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" },
		{ VK_ERROR_VALIDATION_FAILED_EXT , "VK_ERROR_VALIDATION_FAILED_EXT" },
		{ VK_ERROR_INVALID_SHADER_NV , "VK_ERROR_INVALID_SHADER_NV" },
		{ VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT , "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT" },
		{ VK_ERROR_FRAGMENTATION_EXT , "VK_ERROR_FRAGMENTATION_EXT" },
		{ VK_ERROR_NOT_PERMITTED_EXT , "VK_ERROR_NOT_PERMITTED_EXT" },
		{ VK_ERROR_INVALID_DEVICE_ADDRESS_EXT , "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" },
		{ VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT , "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" },
		{ VK_ERROR_OUT_OF_POOL_MEMORY_KHR , "VK_ERROR_OUT_OF_POOL_MEMORY_KHR" },
		{ VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR , "VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR" }
	};

	VkBool32 VKAPI_PTR VulkanDevice::DebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char * pLayerPrefix, const char * pMessage, void * pUserData)
	{
		(void)flags;
		(void)objectType;
		(void)object;
		(void)location;
		(void)messageCode;
		(void)pLayerPrefix;
		(void)pUserData;

		VulkanDevice* pDevice = (VulkanDevice*)pUserData;
		pDevice->_apiErrMsg += StrFormat("%s\n", pMessage);

		OutputDebugString(StrFormat("%s\n", pMessage).data());

		return VK_TRUE;
	}

	VulkanDevice::VulkanDevice()
	{
	}


	VulkanDevice::~VulkanDevice()
	{
	}


	bool VulkanDevice::Create(const IDeviceCreateInfo& info)
	{
		if(!createInstance(info.debugEnabled)) return false;
		if(!createDebugCallback()) return false;;
		if(!pickGpu()) return false;;
		if(!createDevice()) return false;;
		if(!createCommandPool()) return false;;
		if(!createDescriptorPool()) return false;;
		if(!allocCommandBuffers()) return false;;

		return true;
	}

	bool VulkanDevice::Destroy()
	{
		vkFreeCommandBuffers(_device, _cmdPool, 1, &_utilCmd);
		vkDestroyFence(_device, _utilFence, VK_NULL_HANDLE);
		vkDestroyCommandPool(_device, _cmdPool, VK_NULL_HANDLE);

		while (_descriptorPools.size())
		{
			vkDestroyDescriptorPool(_device, _descriptorPools.front(), VK_NULL_HANDLE);
			_descriptorPools.pop_front();
		}

		vkDestroyDevice(_device, VK_NULL_HANDLE);
		vkDestroyInstance(_instance, VK_NULL_HANDLE);

		return true;
	}

	const String & VulkanDevice::GetErrorMsg() const
	{
		return _errMsg;
	}

	bool VulkanDevice::CreateSurfaceKHR(VkWin32SurfaceCreateInfoKHR & info, VkSurfaceKHR * pHandle)
	{
		CheckVkResult(vkCreateWin32SurfaceKHR(_instance, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateSwapchainKHR(VkSwapchainCreateInfoKHR & info, VkSwapchainKHR * pHandle)
	{
		VkBool32 surfaceSupported;
		CheckVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(_gpu, _queue._familyIndex, info.surface, &surfaceSupported));
		if (!surfaceSupported) return false;

		uint formatCount;
		Vector<VkSurfaceFormatKHR> formats;
		CheckVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(_gpu, info.surface, &formatCount, VK_NULL_HANDLE));
		formats.resize(formatCount);
		CheckVkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(_gpu, info.surface, &formatCount, formats.data()));

		bool supportsFormat = false;
		for (uint i = 0; i < formatCount; i++)
		{
			if (formats[i].format == info.imageFormat && formats[i].colorSpace == info.imageColorSpace)
			{
				supportsFormat = true;
				break;
			}
		}
		if (!supportsFormat) return false;


		uint presentModeCount;
		Vector<VkPresentModeKHR> presentModes;
		CheckVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(_gpu, info.surface, &presentModeCount, VK_NULL_HANDLE));
		presentModes.resize(presentModeCount);
		CheckVkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(_gpu, info.surface, &presentModeCount, presentModes.data()));

		bool supportsPresentMode = false;
		for (uint i = 0; i < presentModeCount; i++)
		{
			if (presentModes[i] == info.presentMode)
			{
				supportsPresentMode = true;
				break;
			}
		}
		if (!supportsPresentMode) return false;

		VkSurfaceCapabilitiesKHR capabilites;
		CheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_gpu, info.surface, &capabilites));
		info.minImageCount = capabilites.minImageCount; //TODO should this be max?
		info.pQueueFamilyIndices = &_queue._familyIndex;
		info.queueFamilyIndexCount = 1;

		CheckVkResult(vkCreateSwapchainKHR(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::GetSwapchainImages(VkSwapchainKHR pHandle, Vector<VkImage>& images)
	{
		uint imageCount;
		CheckVkResult(vkGetSwapchainImagesKHR(_device, pHandle, &imageCount, VK_NULL_HANDLE));
		images.resize(imageCount);

		CheckVkResult(vkGetSwapchainImagesKHR(_device, pHandle, &imageCount, images.data()));

		return true;
	}

	bool VulkanDevice::AllocImageMemory(VkImage image, VkMemoryPropertyFlags flags, VkDeviceMemory *pHandle)
	{
		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(_device, image, &req);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = req.size;

		int memIndex = getMemoryIndex(req, flags);
		if (memIndex == -1) return false;

		allocInfo.memoryTypeIndex = memIndex;
		CheckVkResult(vkAllocateMemory(_device, &allocInfo, VK_NULL_HANDLE, pHandle));
		CheckVkResult(vkBindImageMemory(_device, image, *pHandle, 0));

		return true;
	}

	bool VulkanDevice::AllocBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags flags, VkDeviceMemory * pHandle)
	{
		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(_device, buffer, &req);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = req.size;

		int memIndex = getMemoryIndex(req, flags);
		if (memIndex == -1) return false;

		allocInfo.memoryTypeIndex = memIndex;
		CheckVkResult(vkAllocateMemory(_device, &allocInfo, VK_NULL_HANDLE, pHandle));
		CheckVkResult(vkBindBufferMemory(_device, buffer, *pHandle, 0));

		return true;
	}

	bool VulkanDevice::CreateImage(VkImageCreateInfo & info, VkImage * pHandle)
	{
		CheckVkResult(vkCreateImage(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateImageView(VkImageViewCreateInfo & info, VkImageView * pHandle)
	{
		CheckVkResult(vkCreateImageView(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateRenderPass(VkRenderPassCreateInfo & info, VkRenderPass * pHandle)
	{
		CheckVkResult(vkCreateRenderPass(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateFramebuffer(VkFramebufferCreateInfo & info, VkFramebuffer * pHandle)
	{
		CheckVkResult(vkCreateFramebuffer(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateShaderModule(VkShaderModuleCreateInfo & info, VkShaderModule * pHandle)
	{
		CheckVkResult(vkCreateShaderModule(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreatePipelineLayout(VkPipelineLayoutCreateInfo & info, VkPipelineLayout * pHandle)
	{
		CheckVkResult(vkCreatePipelineLayout(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateGraphicsPipeline(VkGraphicsPipelineCreateInfo & info, VkPipeline * pHandle)
	{
		CheckVkResult(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo & info, VkDescriptorSetLayout * pHandle)
	{
		CheckVkResult(vkCreateDescriptorSetLayout(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateSempahore(VkSemaphoreCreateInfo & info, VkSemaphore * pHandle)
	{
		CheckVkResult(vkCreateSemaphore(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateFence(VkFenceCreateInfo & info, VkFence * pHandle)
	{
		CheckVkResult(vkCreateFence(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateBuffer(VkBufferCreateInfo & info, VkBuffer * pHandle)
	{
		CheckVkResult(vkCreateBuffer(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	bool VulkanDevice::CreateSampler(VkSamplerCreateInfo & info, VkSampler * pHandle)
	{
		CheckVkResult(vkCreateSampler(_device, &info, VK_NULL_HANDLE, pHandle));
		return true;
	}

	void VulkanDevice::DestroyBuffer(VkBuffer buffer)
	{
		vkDestroyBuffer(_device, buffer, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyShaderModule(VkShaderModule shader)
	{
		vkDestroyShaderModule(_device, shader, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyDescriptorSetLayout(VkDescriptorSetLayout layout)
	{
		vkDestroyDescriptorSetLayout(_device, layout, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyPipelineLayout(VkPipelineLayout layout)
	{
		vkDestroyPipelineLayout(_device, layout, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroySurface(VkSurfaceKHR surface)
	{
		vkDestroySurfaceKHR(_instance, surface, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroySwapchain(VkSwapchainKHR swapchain)
	{
		vkDestroySwapchainKHR(_device, swapchain, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroySemaphore(VkSemaphore semaphore)
	{
		vkDestroySemaphore(_device, semaphore, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyFence(VkFence fence)
	{
		vkDestroyFence(_device, fence, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyRenderPass(VkRenderPass renderPass)
	{
		vkDestroyRenderPass(_device, renderPass, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyImage(VkImage image)
	{
		vkDestroyImage(_device, image, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyImageView(VkImageView imageView)
	{
		vkDestroyImageView(_device, imageView, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyFramebuffer(VkFramebuffer framebuffer)
	{
		vkDestroyFramebuffer(_device, framebuffer, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroyPipeline(VkPipeline pipeline)
	{
		vkDestroyPipeline(_device, pipeline, VK_NULL_HANDLE);
	}

	void VulkanDevice::DestroySampler(VkSampler sampler)
	{
		vkDestroySampler(_device, sampler, VK_NULL_HANDLE);
	}

	void VulkanDevice::FreeDescriptorSet(VkDescriptorSet set)
	{
		//vkFreeDescriptorSets(_device, _descriptorPool, 1, &set);
	}

	bool VulkanDevice::AcquireNextImage(VkSwapchainKHR swapchain, VkSemaphore semaphore, uint64_t timeout, VkFence fence, uint * pImgIndex)
	{
		CheckVkResult(vkAcquireNextImageKHR(_device, swapchain, timeout, semaphore, fence, pImgIndex));
		return true;
	}

	bool VulkanDevice::AllocateCommandBuffer(VkCommandBuffer * pHandle)
	{
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 1;
		info.commandPool = _cmdPool;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		
		CheckVkResult(vkAllocateCommandBuffers(_device, &info, pHandle));
		return true;
	}

	bool VulkanDevice::QueueSubmit(VkSubmitInfo * pInfos, uint count, VkFence fence)
	{
		CheckVkResult(vkQueueSubmit(_queue._queue, count, pInfos, fence));
		return true;
	}

	bool VulkanDevice::QueuePresent(VkPresentInfoKHR & info)
	{
		CheckVkResult(vkQueuePresentKHR(_queue._queue, &info));
		return true;
	}

	bool VulkanDevice::ProcessFences(VkFence * pFences, uint count, uint64_t timeout, bool waitForall)
	{
		CheckVkResult(vkWaitForFences(_device, count, pFences, waitForall, timeout));
		CheckVkResult(vkResetFences(_device, count, pFences));
		return true;
	}

	bool VulkanDevice::TransferBufferData(VkBuffer buffer, const void * pData, uint size)
	{
		VkCommandBufferBeginInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkBuffer transferSrc;
		VkDeviceMemory transferMem;
		VkBufferCreateInfo transferInfo = {};
		transferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		transferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		transferInfo.size = size;
		transferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (!CreateBuffer(transferInfo, &transferSrc)) return false;
		if (!AllocBufferMemory(transferSrc, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &transferMem)) return false;

		void *deviceMem = 0;
		CheckVkResult(vkMapMemory(_device, transferMem, 0, size, 0, &deviceMem));
		memcpy(deviceMem, pData, size);
		vkUnmapMemory(_device, transferMem);

		CheckVkResult(vkBeginCommandBuffer(_utilCmd, &cmdInfo));
		{
			VkBufferCopy buffCopy = {};
			buffCopy.size = size;
			vkCmdCopyBuffer(_utilCmd, transferSrc, buffer, 1, &buffCopy);
		}
		CheckVkResult(vkEndCommandBuffer(_utilCmd));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_utilCmd;
		CheckVkResult(vkQueueSubmit(_queue._queue, 1, &submitInfo, _utilFence));
		if (!ProcessFences(&_utilFence, 1, UINT64_MAX, true)) return false;

		FreeMemory(transferMem);
		DestroyBuffer(transferSrc);

		return true;
	}

	bool VulkanDevice::AllocateDescriptorSets(VkDescriptorSetAllocateInfo & info, VkDescriptorSet * pHandle)
	{
		info.descriptorPool = _descriptorPools.front();
		VkResult result = vkAllocateDescriptorSets(_device, &info, pHandle);
		if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
		{
			createDescriptorPool();
			info.descriptorPool = _descriptorPools.front();
			result = vkAllocateDescriptorSets(_device, &info, pHandle);
		}
		CheckVkResult(result);

		return true;
	}

	bool VulkanDevice::UpdateDescriptorSets(VkWriteDescriptorSet *pWriteSets, uint count)
	{
		vkUpdateDescriptorSets(_device, count, pWriteSets, 0, 0);
		return true;
	}

	bool VulkanDevice::MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData)
	{
		CheckVkResult(vkMapMemory(_device, memory, offset, size, flags, ppData));
		return true;
	}

	bool VulkanDevice::UnmapMemory(VkDeviceMemory memory)
	{
		vkUnmapMemory(_device, memory);
		return true;
	}

	bool VulkanDevice::TransferImageData(VkImage image, const ImageData& baseImg, uint mipCount, const ImageData* pMipData)
	{
		Vector<ImageData> images;
		images.resize(1 + mipCount);
		images[0] = baseImg;

		if(mipCount)
			memcpy(&images[1], pMipData, sizeof(ImageData) * mipCount);

		if (images.size() > 1)
		{
			int ww = 5;
			ww++;
		}

		uint size = 0;
		for (uint i = 0; i < images.size(); i++)
		{
			size += images[i].GetImageSize();
		}

		VkCommandBufferBeginInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkBuffer transferSrc;
		VkDeviceMemory transferMem;
		VkBufferCreateInfo transferInfo = {};
		transferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		transferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		transferInfo.size = size;
		transferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (!CreateBuffer(transferInfo, &transferSrc)) return false;
		if (!AllocBufferMemory(transferSrc, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &transferMem)) return false;

		void *deviceMem = 0;
		CheckVkResult(vkMapMemory(_device, transferMem, 0, size, 0, &deviceMem));
		usize memAddr = (usize)deviceMem;
		for (uint i = 0; i < images.size(); i++)
		{
			const ImageData& img = images[i];
			uint imgSize = img.GetImageSize();
			memcpy((void*)memAddr, img.Pixels, imgSize);
			memAddr += imgSize;
		}
		vkUnmapMemory(_device, transferMem);

		CheckVkResult(vkBeginCommandBuffer(_utilCmd, &cmdInfo));
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = images.size();

			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.srcAccessMask = 0;
			vkCmdPipelineBarrier(_utilCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &barrier);

			uint imgOffset = 0;
			for (uint i = 0; i < images.size(); i++)
			{
				const ImageData& img = images[i];

				VkBufferImageCopy buffCopy = {};
				//buffCopy.bufferImageHeight = img.Height;
				//buffCopy.bufferRowLength = img.Width;
				buffCopy.bufferOffset = imgOffset;
				buffCopy.imageExtent.width = img.Width;
				buffCopy.imageExtent.height = img.Height;
				buffCopy.imageExtent.depth = 1;
				buffCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				buffCopy.imageSubresource.layerCount = 1;
				buffCopy.imageSubresource.mipLevel = i;
				vkCmdCopyBufferToImage(_utilCmd, transferSrc, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffCopy);

				imgOffset += img.GetImageSize();
			}

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(_utilCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &barrier);
		}
		CheckVkResult(vkEndCommandBuffer(_utilCmd));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_utilCmd;
		CheckVkResult(vkQueueSubmit(_queue._queue, 1, &submitInfo, _utilFence));
		if (!ProcessFences(&_utilFence, 1, UINT64_MAX, true)) return false;

		FreeMemory(transferMem);
		DestroyBuffer(transferSrc);

		return true;
	}

	bool VulkanDevice::TransferImageData(VkImage image, const ImageData* images, uint imageCount)
	{
		uint size = 0;
		for (uint i = 0; i < imageCount; i++)
		{
			size += images[i].GetImageSize();
		}

		VkCommandBufferBeginInfo cmdInfo = {};
		cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkBuffer transferSrc;
		VkDeviceMemory transferMem;
		VkBufferCreateInfo transferInfo = {};
		transferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		transferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		transferInfo.size = size;
		transferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		if (!CreateBuffer(transferInfo, &transferSrc)) return false;
		if (!AllocBufferMemory(transferSrc, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &transferMem)) return false;

		void* deviceMem = 0;
		CheckVkResult(vkMapMemory(_device, transferMem, 0, size, 0, &deviceMem));
		usize memAddr = (usize)deviceMem;
		for (uint i = 0; i < imageCount; i++)
		{
			const ImageData& img = images[i];
			uint imgSize = img.GetImageSize();
			memcpy((void*)memAddr, img.Pixels, imgSize);
			memAddr += imgSize;
		}
		vkUnmapMemory(_device, transferMem);

		CheckVkResult(vkBeginCommandBuffer(_utilCmd, &cmdInfo));
		{
			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.layerCount = 6;
			barrier.subresourceRange.levelCount = 1;

			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.srcAccessMask = 0;
			vkCmdPipelineBarrier(_utilCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &barrier);

			uint imgOffset = 0;
			for (uint i = 0; i < imageCount; i++)
			{
				const ImageData& img = images[i];

				VkBufferImageCopy buffCopy = {};
				//buffCopy.bufferImageHeight = img.Height;
				//buffCopy.bufferRowLength = img.Width;
				buffCopy.bufferOffset = imgOffset;
				buffCopy.imageExtent.width = img.Width;
				buffCopy.imageExtent.height = img.Height;
				buffCopy.imageExtent.depth = 1;
				buffCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				buffCopy.imageSubresource.layerCount = 1;
				buffCopy.imageSubresource.baseArrayLayer = i;
				vkCmdCopyBufferToImage(_utilCmd, transferSrc, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffCopy);

				imgOffset += img.GetImageSize();
			}

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(_utilCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1, &barrier);
		}
		CheckVkResult(vkEndCommandBuffer(_utilCmd));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_utilCmd;
		CheckVkResult(vkQueueSubmit(_queue._queue, 1, &submitInfo, _utilFence));
		if (!ProcessFences(&_utilFence, 1, UINT64_MAX, true)) return false;

		FreeMemory(transferMem);
		DestroyBuffer(transferSrc);
	}

	bool VulkanDevice::FreeMemory(VkDeviceMemory memory)
	{
		vkFreeMemory(_device, memory, VK_NULL_HANDLE);
		return true;
	}

	bool VulkanDevice::FreeCommandBuffer(VkCommandBuffer cmdBuffer)
	{
		vkFreeCommandBuffers(_device, _cmdPool, 1, &cmdBuffer);
		return true;
	}

	bool VulkanDevice::WaitIdle()
	{
		CheckVkResult(vkDeviceWaitIdle(_device));
		return true;
	}

	uint VulkanDevice::GetUniformBufferMaxSize() const
	{
		return _gpuProps.limits.maxUniformBufferRange;
	}

	uint VulkanDevice::GetMinUniformBufferAlignment() const
	{
		return (uint)_gpuProps.limits.minUniformBufferOffsetAlignment;
	}

	bool VulkanDevice::ContainsInstanceExtension(const char* extensionName) const
	{
		for (uint i = 0; i < _instanceExtensions.size(); i++)
		{
			if (strcmp(extensionName, _instanceExtensions[i]) == 0)
				return true;
		}

		return false;
	}

	bool VulkanDevice::ContainsDeviceExtension(const char* extensionName) const
	{
		for (uint i = 0; i < _deviceExtensions.size(); i++)
		{
			if (strcmp(extensionName, _deviceExtensions[i]) == 0)
				return true;
		}

		return false;
	}

	String VulkanDevice::QueryAPIError()
	{
		String ret = _apiErrMsg;
		_apiErrMsg.clear();
		return ret;
	}

	bool VulkanDevice::createInstance(bool debugEnabled)
	{
		VkApplicationInfo app = {};
		app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app.apiVersion = VK_API_VERSION_1_1;
		app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app.pApplicationName = 0;
		app.pEngineName = 0;

		_instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
		_instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

		if(debugEnabled)
		{
			_instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			_validationLayers.push_back("VK_LAYER_KHRONOS_validation");
		}

		VkInstanceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		info.pApplicationInfo = &app;
		info.enabledExtensionCount = _instanceExtensions.size();
		info.ppEnabledExtensionNames = _instanceExtensions.data();
		info.enabledLayerCount = _validationLayers.size();
		info.ppEnabledLayerNames = _validationLayers.data();

		CheckVkResult(vkCreateInstance(&info, VK_NULL_HANDLE, &_instance));
		return true;
	}

	bool VulkanDevice::createAllocationCallback()
	{
		//VkAllocationCallbacks callbacks;

		//callbacks.pfnAllocation

		return false;
	}

	bool VulkanDevice::createDebugCallback()
	{
		if (ContainsInstanceExtension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
		{
			VkDebugReportCallbackCreateInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			info.pfnCallback = DebugCallback;
			info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
			info.pUserData = this;

			PFN_vkCreateDebugReportCallbackEXT createFunc = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugReportCallbackEXT");

			CheckVkResult(createFunc(_instance, &info, VK_NULL_HANDLE, &_debugCallback));
		}

		return true;
	}

	bool VulkanDevice::createDevice()
	{
		VkDeviceQueueCreateInfo queue = {};
		queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue.queueFamilyIndex = _queue._familyIndex;
		queue.queueCount = 1;
		float priority = 1.0f;
		queue.pQueuePriorities = &priority;

		_deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkDeviceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		info.enabledExtensionCount = _deviceExtensions.size();
		info.ppEnabledExtensionNames = _deviceExtensions.data();
		info.enabledLayerCount = _validationLayers.size();
		info.ppEnabledLayerNames = _validationLayers.data();
		info.queueCreateInfoCount = 1;
		info.pQueueCreateInfos = &queue;

		VkPhysicalDeviceFeatures features = {};
		features.geometryShader = VK_TRUE;
		features.samplerAnisotropy = VK_TRUE;
		info.pEnabledFeatures = &features;

		VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingExt = {};
		if (ContainsDeviceExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME))
		{
			descriptorIndexingExt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
			descriptorIndexingExt.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
			info.pNext = &descriptorIndexingExt;
		}

		CheckVkResult(vkCreateDevice(_gpu, &info, VK_NULL_HANDLE, &_device));

		vkGetDeviceQueue(_device, _queue._familyIndex, 0, &_queue._queue);

		return true;
	}


	bool VulkanDevice::pickGpu()
	{
		uint numDevices;
		CheckVkResult(vkEnumeratePhysicalDevices(_instance, &numDevices, VK_NULL_HANDLE));

		if (numDevices == 0)
			return false;

		Vector<VkPhysicalDevice> gpus;
		gpus.resize(numDevices);

		vkEnumeratePhysicalDevices(_instance, &numDevices, gpus.data());

		uint bestScore = 0;

		for (uint i = 0; i < numDevices; i++)
		{
			uint score = 0;

			VkPhysicalDevice gpu = gpus[i];

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(gpu, &features);

			assert(features.fillModeNonSolid == 1);

			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(gpu, &props);

			VkPhysicalDeviceLimits limits = props.limits;

			score += limits.maxColorAttachments;
			score += limits.maxFramebufferWidth;
			score += limits.maxFramebufferHeight;
			score += limits.maxPushConstantsSize;
			score += limits.maxUniformBufferRange;
			score += limits.maxImageDimension2D;
			score += limits.maxVertexInputAttributes;
			score += limits.maxBoundDescriptorSets;

			if (score > bestScore)
			{
				_gpu = gpu;
				_gpuProps = props;
				bestScore = score;
			}
		}

		vkGetPhysicalDeviceMemoryProperties(_gpu, &_gpuMem);

		uint numQueues;
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &numQueues, VK_NULL_HANDLE);

		Vector<VkQueueFamilyProperties> queueProps;
		queueProps.resize(numQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &numQueues, queueProps.data());

		for (uint i = 0; i < queueProps.size(); i++)
		{
			VkQueueFamilyProperties queue = queueProps[i];
			if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				_queue._familyIndex = i;
				break;
			}

			//VkSurfaceCapabilitiesKHR surface;
			//vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_gpu, VK_NULL_HANDLE, &surface);
			//surface.
		}

		return true;
	}


	bool VulkanDevice::createCommandPool()
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.queueFamilyIndex = _queue._familyIndex;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		CheckVkResult(vkCreateCommandPool(_device, &info, VK_NULL_HANDLE, &_cmdPool));

		return true;
	}

	bool VulkanDevice::createDescriptorPool()
	{
		VkDescriptorPoolSize sizes[3];

		sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		sizes[0].descriptorCount = 2048;

		sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		sizes[1].descriptorCount = 2048;

		sizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		sizes[2].descriptorCount = 2048;

		VkDescriptorPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets = 2048;
		info.poolSizeCount = sizeof(sizes) / sizeof(*sizes);
		info.pPoolSizes = sizes;
		//info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; //is this not effecient? using this works better for my binding scheme atm..

		VkDescriptorPool pool = VK_NULL_HANDLE;
		CheckVkResult(vkCreateDescriptorPool(_device, &info, VK_NULL_HANDLE, &pool));

		_descriptorPools.push_front(pool);
		return true;
	}

	bool VulkanDevice::allocCommandBuffers()
	{
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 2;
		info.commandPool = _cmdPool;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		CheckVkResult(vkAllocateCommandBuffers(_device, &info, &_utilCmd));

		VkFenceCreateInfo utilFence = {};
		utilFence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		if (!CreateFence(utilFence, &_utilFence)) return false;

		return true;
	}

	int VulkanDevice::getMemoryIndex(VkMemoryRequirements memReq, VkMemoryPropertyFlags memProps)
	{
		uint bits = memReq.memoryTypeBits;
		for (uint i = 0; i < _gpuMem.memoryTypeCount; i++)
		{
			if (_gpuMem.memoryTypes[i].propertyFlags & memProps)
			{
				if ((bits & 1) == 1)
				{
					return i;
				}
			}
			bits >>= 1;
		}

		return -1;
	}
}