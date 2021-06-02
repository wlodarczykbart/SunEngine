#pragma once

#include "ISurface.h"
#include "VulkanRenderOutput.h"

namespace SunEngine
{
	class VulkanCommandBuffer;

	class VulkanSurface : public VulkanRenderOutput, public ISurface
	{
	public:
		VulkanSurface();
		~VulkanSurface();

		bool Create(GraphicsWindow *window) override;
		bool Destroy() override;

		bool StartFrame(ICommandBuffer * cmdBuffer) override;
		bool SubmitFrame(ICommandBuffer * cmdBuffer) override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		uint GetBackBufferCount() const override;

	private:
		bool createSurface();
		bool createSwapchain();
		bool createRenderPass();
		bool createFrames();

		struct Frame
		{
			VkImage _image;
			VkImageView _imageView;
			VkFramebuffer _framebuffer;

			VkImage _depthImage;
			VulkanDevice::MemoryHandle _depthMem;
			VkImageView _depthView;

			VkSemaphore _imgAvailableSemaphore;
			VkSemaphore _renderFinishedSemaphore;
			VkFence _renderFinishedFence;

			uint _swapImageIndex;
		};

		GraphicsWindow* _window;
		VkSurfaceKHR _surface;
		VkSwapchainKHR _swapchain;

		VkFormat _format;
		Vector<Frame> _frames;

		uint _frameIndex;
	};

}