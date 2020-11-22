#pragma once

#include "ICommandBuffer.h"
#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanUniformBuffer;

	class VulkanCommandBuffer : public VulkanObject, public ICommandBuffer
	{
	public:
		VulkanCommandBuffer();
		~VulkanCommandBuffer();

		void Create() override;
		void Begin() override;
		void End() override;
		void Submit() override;

		void BeginRenderPass(const VkRenderPassBeginInfo &info);
		void EndRenderPass();
		void BindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline);
		void BindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint firstSet, uint setCount, VkDescriptorSet *pSets, uint dynamicOffsetCount, uint* pDynamicOffsets);

		void BindVertexBuffer(VkBuffer buffer, VkDeviceSize offset);
		void BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);

		void Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance) override;
		void DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance) override;
		void SetScissor(float x, float y, float width, float height) override;
		void SetViewport(float x, float y, float width, float height) override;

		//void SetViewport(const VkViewport &viewport);
		//void SetScissor(const VkRect2D &rect);

		VkPipeline GetCurrentPipeline() const;
		VkRenderPass GetCurrentRenderPass() const;
		VkFramebuffer GetCurrentFramebuffer() const;

		bool ReigsterUniformBuffer(VulkanUniformBuffer* pBuffer);

	private:
		friend class VulkanSurface;
		VkCommandBuffer _cmdBuffer;
		VkFence _fence;
	
		VkRenderPassBeginInfo _currentRenderPass;
		VkPipeline _currentPipeline;

		HashSet<VulkanUniformBuffer*> _activeBuffers;
	};
}