#pragma once

#include "GraphicsAPIDef.h"

#include "Types.h"
#include "GraphicsWindow.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_win32.h"
#include "IDevice.h"

namespace SunEngine
{
	class VulkanObject;

	class VulkanDevice : public IDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice();

		bool Create() override;
		bool Destroy() override;

		const String &GetErrorMsg() const override;
		String QueryAPIError() override;

		bool CreateSurfaceKHR(VkWin32SurfaceCreateInfoKHR &info, VkSurfaceKHR *pHandle);
		bool CreateSwapchainKHR(VkSwapchainCreateInfoKHR &info, VkSwapchainKHR *pHandle);
		bool CreateImage(VkImageCreateInfo &info, VkImage *pHandle);
		bool CreateImageView(VkImageViewCreateInfo &info, VkImageView *pHandle);
		bool CreateRenderPass(VkRenderPassCreateInfo &info, VkRenderPass *pHandle);
		bool CreateFramebuffer(VkFramebufferCreateInfo &info, VkFramebuffer *pHandle);
		bool CreateShaderModule(VkShaderModuleCreateInfo &info, VkShaderModule *pHandle);
		bool CreatePipelineLayout(VkPipelineLayoutCreateInfo &info, VkPipelineLayout *pHandle);
		bool CreateGraphicsPipeline(VkGraphicsPipelineCreateInfo &info, VkPipeline *pHandle);
		bool CreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo &info, VkDescriptorSetLayout *pHandle);
		bool CreateSempahore(VkSemaphoreCreateInfo &info, VkSemaphore *pHandle);
		bool CreateFence(VkFenceCreateInfo &info, VkFence *pHandle);
		bool CreateBuffer(VkBufferCreateInfo &info, VkBuffer *pHandle);
		bool CreateSampler(VkSamplerCreateInfo &info, VkSampler *pHandle);

		void DestroyBuffer(VkBuffer buffer);
		void DestroyShaderModule(VkShaderModule shader);
		void DestroyDescriptorSetLayout(VkDescriptorSetLayout layout);
		void DestroyPipelineLayout(VkPipelineLayout layout);
		void DestroySurface(VkSurfaceKHR surface);
		void DestroySwapchain(VkSwapchainKHR swapchain);
		void DestroySemaphore(VkSemaphore semaphore);
		void DestroyFence(VkFence fence);
		void DestroyRenderPass(VkRenderPass renderPass);
		void DestroyImage(VkImage image);
		void DestroyImageView(VkImageView imageView);
		void DestroyFramebuffer(VkFramebuffer framebuffer);
		void DestroyPipeline(VkPipeline pipeline);
		void DestroySampler(VkSampler sampler);

		void FreeDescriptorSet(VkDescriptorSet set);


		bool GetSwapchainImages(VkSwapchainKHR pHandle, Vector<VkImage> &images);
		bool AllocImageMemory(VkImage image, VkMemoryPropertyFlags flags, VkDeviceMemory *pHandle);
		bool AllocBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags flags, VkDeviceMemory *pHandle);
		bool AcquireNextImage(VkSwapchainKHR swapchain, VkSemaphore semaphore, uint64_t timeout, VkFence fence, uint* pImgIndex);
		bool AllocateCommandBuffer(VkCommandBuffer *pHandle);
		bool QueueSubmit(VkSubmitInfo *pInfos, uint count, VkFence fence);
		bool QueuePresent(VkPresentInfoKHR &info);
		bool ProcessFences(VkFence *pFences, uint count, uint64_t timeout, bool waitForall);
		bool TransferBufferData(VkBuffer buffer, const void* pData, uint size);
		bool AllocateDescriptorSets(VkDescriptorSetAllocateInfo &info, VkDescriptorSet* pHandle);
		bool UpdateDescriptorSets(VkWriteDescriptorSet *pWriteSets, uint count);
		bool MapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData);
		bool UnmapMemory(VkDeviceMemory memory);
		bool TransferImageData(VkImage image, const ImageData& baseImg, uint mipCount, const ImageData* pMipData);
		bool FreeMemory(VkDeviceMemory memory);
		bool FreeCommandBuffer(VkCommandBuffer cmdBuffer);

		bool WaitIdle();

		uint GetUniformBufferMaxSize() const;
		uint GetMinUniformBufferAlignment() const;

		bool ContainsExtension(const char* extensionName) const;

	private:
		static VkBool32 VKAPI_PTR DebugCallback(
			VkDebugReportFlagsEXT                       flags,
			VkDebugReportObjectTypeEXT                  objectType,
			uint64_t                                    object,
			size_t                                      location,
			int32_t                                     messageCode,
			const char*                                 pLayerPrefix,
			const char*                                 pMessage,
			void*                                       pUserData);

		bool createInstance();
		bool createAllocationCallback();
		bool createDebugCallback();
		bool createDevice();
		bool pickGpu();
		bool createCommandPool();
		bool createDescriptorPool();
		bool allocCommandBuffers();
		int getMemoryIndex(VkMemoryRequirements memReq, VkMemoryPropertyFlags memProps);

		struct GpuQueue
		{
			VkQueue _queue;
			uint _familyIndex;
		};

		VkInstance _instance;
		VkPhysicalDevice _gpu;
		VkPhysicalDeviceMemoryProperties _gpuMem;
		VkPhysicalDeviceProperties _gpuProps;
		VkDevice _device;
		VkCommandPool _cmdPool;
		LinkedList<VkDescriptorPool> _descriptorPools;
		VkDebugReportCallbackEXT _debugCallback;
		GpuQueue _queue;

		VkCommandBuffer _utilCmd;
		VkFence _utilFence;

		String _errMsg;
		String _apiErrMsg;
		String _errLine;
	};

}