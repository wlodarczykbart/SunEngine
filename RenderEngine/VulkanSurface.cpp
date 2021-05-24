#include "VulkanCommandBuffer.h"

#include "VulkanSurface.h"

namespace SunEngine
{

	VulkanSurface::VulkanSurface() 
	{
		_frameIndex = 0;
		_swapchain = VK_NULL_HANDLE;
		_format = VK_FORMAT_UNDEFINED;
		_window = 0;
	}


	VulkanSurface::~VulkanSurface()
	{
	}

	bool VulkanSurface::Create(GraphicsWindow * window)
	{
		_window = window;

		_extent.width = window->Width();
		_extent.height = window->Height();

		if (!createSurface()) return false;
		if (!createSwapchain()) return false;
		if (!createRenderPass()) return false;
		if (!createFrames()) return false;

		return true;
	}

	bool VulkanSurface::Destroy()
	{
		_device->WaitIdle();

		for (uint i = 0; i < _frames.size(); i++)
		{
			Frame& frame = _frames[i];	
			_device->DestroyFramebuffer(frame._framebuffer);
			_device->DestroyImageView(frame._imageView);
			_device->DestroyImage(frame._image);	
			_device->DestroyImageView(frame._depthView);
			_device->FreeMemory(frame._depthMem);
			_device->DestroyImage(frame._depthImage);
			_device->DestroyFence(frame._renderFinishedFence);
			_device->DestroySemaphore(frame._renderFinishedSemaphore);
			_device->DestroySemaphore(frame._imgAvailableSemaphore);
		}

		_device->DestroyRenderPass(_renderPass);
		//_device->DestroySwapchain(_swapchain); //TODO: causes crash
		//_device->DestroySurface(_surface);

		return true;
	}

	bool VulkanSurface::StartFrame(ICommandBuffer * cmdBuffer)
	{
		Frame& frame = _frames[_frameIndex];

		if (!_device->ProcessFences(&frame._renderFinishedFence, 1, UINT64_MAX, true)) return false;
		if (!_device->AcquireNextImage(_swapchain, frame._imgAvailableSemaphore, UINT64_MAX, VK_NULL_HANDLE, &frame._swapImageIndex)) return false;

		cmdBuffer->Begin();
		return true;
	}

	bool VulkanSurface::SubmitFrame(ICommandBuffer * cmdBuffer)
	{
		cmdBuffer->End();

		auto frame = _frames[_frameIndex];

		VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &static_cast<VulkanCommandBuffer*>(cmdBuffer)->_cmdBuffer;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &frame._imgAvailableSemaphore;
		submitInfo.pWaitDstStageMask = &waitFlags;
		submitInfo.pSignalSemaphores = &frame._renderFinishedSemaphore;
		submitInfo.signalSemaphoreCount = 1;

		if (!_device->QueueSubmit(&submitInfo, 1, frame._renderFinishedFence)) return false;

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pWaitSemaphores = &frame._renderFinishedSemaphore;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pSwapchains = &_swapchain;
		presentInfo.swapchainCount = 1;
		presentInfo.pImageIndices = &frame._swapImageIndex;
		
		if (!_device->QueuePresent(presentInfo)) return false;

		_frameIndex++;
		_frameIndex %= _frames.size();

		return true;
	}

	void VulkanSurface::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		VkClearValue clearValues[2];

		clearValues[0].color.float32[0] = 0.5f;
		clearValues[0].color.float32[1] = 0.2f;
		clearValues[0].color.float32[2] = 0.9f;
		clearValues[0].color.float32[3] = 1.f;

		clearValues[1].depthStencil.depth = 1.0f;
		clearValues[1].depthStencil.stencil = 0xff;

		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.clearValueCount = 2;
		info.pClearValues = clearValues;
		info.renderPass = _renderPass;
		info.renderArea.extent.width = _window->Width();
		info.renderArea.extent.height = _window->Height();
		info.framebuffer = _frames[_frameIndex]._framebuffer;

		static_cast<VulkanCommandBuffer*>(cmdBuffer)->BeginRenderPass(info, 1, SE_MSAA_OFF);
	}

	void VulkanSurface::Unbind(ICommandBuffer * cmdBuffer)
	{
		static_cast<VulkanCommandBuffer*>(cmdBuffer)->EndRenderPass();
	}

	uint VulkanSurface::GetBackBufferCount() const
	{
		return _frames.size();
	}

	bool VulkanSurface::createSurface()
	{
		VkWin32SurfaceCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		info.hinstance = GetModuleHandle(0);
		info.hwnd = _window->Handle();

		if (!_device->CreateSurfaceKHR(info, &_surface)) return false;

		return true;
	}

	bool VulkanSurface::createSwapchain()
	{
		VkSwapchainCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.imageExtent = _extent;
		info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
		info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		info.imageArrayLayers = 1;
		info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		info.presentMode = VK_PRESENT_MODE_FIFO_KHR; //BART NOTE: defaulting to vsync...
		info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		info.surface = _surface;

		if (!_device->CreateSwapchainKHR(info, &_swapchain)) return false;

		_format = info.imageFormat;

		return true;
	}

	bool VulkanSurface::createRenderPass()
	{
		VkAttachmentDescription attachments[2];
		uint numAttachments = 2;

		VkAttachmentDescription colorAttachment = {};
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0] = colorAttachment;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.format = VK_FORMAT_D24_UNORM_S8_UINT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1] = depthAttachment;

		VkAttachmentReference colorRefAttachment = {};
		colorRefAttachment.attachment = 0;
		colorRefAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthRefAttachment = {};
		depthRefAttachment.attachment = 1;
		depthRefAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRefAttachment;
		subpass.pDepthStencilAttachment = &depthRefAttachment;

		Vector<VkSubpassDependency> dependencies;
		dependencies.resize(2);

		// First dependency at the start of the renderpass
		// Does the transition from final to initial layout 
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;								// Producer of the dependency 
		dependencies[0].dstSubpass = 0;													// Consumer is our single subpass that will wait for the execution depdendency
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Second dependency at the end the renderpass
		// Does the transition from the initial to the final layout
		dependencies[1].srcSubpass = 0;													// Producer of the dependency is our single subpass
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;								// Consumer are all commands outside of the renderpass
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.pAttachments = attachments;
		info.attachmentCount = numAttachments;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = dependencies.size();
		info.pDependencies = dependencies.data();

		if (!_device->CreateRenderPass(info, &_renderPass)) return false;

		return true;
	}

	bool VulkanSurface::createFrames()
	{
		Vector<VkImage> images;
		if (!_device->GetSwapchainImages(_swapchain, images)) return false;

		_frames.resize(images.size());

		VkImageView attachments[2];
		uint numAttachments = 2;

		for (uint i = 0; i < _frames.size(); i++)
		{
			Frame frame;
			frame._image = images[i];

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = frame._image;
			viewInfo.format = _format;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

			if (!_device->CreateImageView(viewInfo, &frame._imageView)) return false;

			VkImageCreateInfo depthInfo = {};
			depthInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			depthInfo.extent.width = _extent.width;
			depthInfo.extent.height = _extent.height;
			depthInfo.extent.depth = 1;
			depthInfo.arrayLayers = 1;
			depthInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
			depthInfo.imageType = VK_IMAGE_TYPE_2D;
			depthInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthInfo.mipLevels = 1;
			depthInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			depthInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			depthInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			depthInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			if (!_device->CreateImage(depthInfo, &frame._depthImage)) return false;
			if (!_device->AllocImageMemory(frame._depthImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &frame._depthMem)) return false;

			VkImageViewCreateInfo depthViewInfo = {};
			depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			depthViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			depthViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
			depthViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
			depthViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
			depthViewInfo.format = depthInfo.format;
			depthViewInfo.image = frame._depthImage;
			depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			depthViewInfo.subresourceRange.layerCount = 1;
			depthViewInfo.subresourceRange.levelCount = 1;
			depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			if (!_device->CreateImageView(depthViewInfo, &frame._depthView)) return false;


			attachments[0] = frame._imageView;
			attachments[1] = frame._depthView;
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.attachmentCount = numAttachments;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.layers = 1;
			framebufferInfo.width = _extent.width;
			framebufferInfo.height = _extent.height;
			framebufferInfo.renderPass = _renderPass;
			if (!_device->CreateFramebuffer(framebufferInfo, &frame._framebuffer)) return false;

			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			if (!_device->CreateSempahore(semaphoreInfo, &frame._imgAvailableSemaphore)) return false;
			if (!_device->CreateSempahore(semaphoreInfo, &frame._renderFinishedSemaphore)) return false;

			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			if (!_device->CreateFence(fenceInfo, &frame._renderFinishedFence)) return false;

			_frames[i] = frame;
		}

		return true;
	}
}