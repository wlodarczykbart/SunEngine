#include "VulkanUniformBuffer.h"
#include "VulkanCommandBuffer.h"

namespace SunEngine
{
	VulkanCommandBuffer::VulkanCommandBuffer() 
	{
		_currentRenderPass = {};
		_currentPipeline = VK_NULL_HANDLE;
		_cmdBuffer = VK_NULL_HANDLE;
		_fence = VK_NULL_HANDLE;
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		_device->FreeCommandBuffer(_cmdBuffer);
		_device->DestroyFence(_fence);
	}

	void VulkanCommandBuffer::Create()
	{
		_device->AllocateCommandBuffer(&_cmdBuffer);

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		//fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		_device->CreateFence(fenceInfo, &_fence);
	}

	void VulkanCommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(_cmdBuffer, &info);		
	}

	void VulkanCommandBuffer::End()
	{
		vkEndCommandBuffer(_cmdBuffer);
	}

	void VulkanCommandBuffer::Submit()
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_cmdBuffer;

		_device->QueueSubmit(&submitInfo, 1, _fence);
		_device->ProcessFences(&_fence, 1, UINT64_MAX, true);
	}

	void VulkanCommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo & info)
	{
		vkCmdBeginRenderPass(_cmdBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
		
		_currentRenderPass = info;

		VkViewport viewport = {};
		viewport.width = (float)_currentRenderPass.renderArea.extent.width;
		viewport.height = -(float)_currentRenderPass.renderArea.extent.height;
		viewport.y = (float)_currentRenderPass.renderArea.extent.height;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.extent = _currentRenderPass.renderArea.extent;

		vkCmdSetViewport(_cmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(_cmdBuffer, 0, 1, &scissor);
	}

	void VulkanCommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(_cmdBuffer);

		_currentRenderPass = {};
		_currentPipeline = VK_NULL_HANDLE;
	}

	void VulkanCommandBuffer::BindPipeline(VkPipelineBindPoint bindPoint, VkPipeline pipeline)
	{
		vkCmdBindPipeline(_cmdBuffer, bindPoint, pipeline);
		_currentPipeline = pipeline;
	}

	void VulkanCommandBuffer::BindDescriptorSets(VkPipelineBindPoint bindPoint, VkPipelineLayout layout, uint firstSet, uint setCount, VkDescriptorSet *pSets, uint dynamicOffsetCount, uint* pDynamicOffsets)
	{
		(void)dynamicOffsetCount;
		(void)pDynamicOffsets;
		vkCmdBindDescriptorSets(_cmdBuffer, bindPoint, layout, firstSet, setCount, pSets, dynamicOffsetCount, pDynamicOffsets);
	}

	void VulkanCommandBuffer::BindVertexBuffer(VkBuffer buffer, VkDeviceSize offset)
	{
		vkCmdBindVertexBuffers(_cmdBuffer, 0, 1, &buffer, &offset);
	}

	void VulkanCommandBuffer::BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
	{
		vkCmdBindIndexBuffer(_cmdBuffer, buffer, offset, indexType);
	}

	void VulkanCommandBuffer::Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
	{
		vkCmdDraw(_cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanCommandBuffer::DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance)
	{
		vkCmdDrawIndexed(_cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	//void VulkanCommandBuffer::SetViewport(const VkViewport& viewport)
	//{
	//	vkCmdSetViewport(_cmdBuffer, 0, 1, &viewport);
	//}

	//void VulkanCommandBuffer::SetScissor(const VkRect2D &rect)
	//{
	//	vkCmdSetScissor(_cmdBuffer, 0, 1, &rect);
	//}

	void VulkanCommandBuffer::SetScissor(float x, float y, float width, float height)
	{
		VkRect2D rect;
		rect.extent = { (uint)width, (uint)height };
		rect.offset = { (int)x, (int)y };
		vkCmdSetScissor(_cmdBuffer, 0, 1, &rect);
	}

	void VulkanCommandBuffer::SetViewport(float x, float y, float width, float height)
	{
		VkViewport viewport = {};
		viewport.x = x;
		viewport.width = (float)width;
		viewport.height = -(float)height;
		viewport.y = (float)height - y; //???? TODO
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(_cmdBuffer, 0, 1, &viewport);
	}

	VkPipeline VulkanCommandBuffer::GetCurrentPipeline() const
	{
		return _currentPipeline;
	}

	VkRenderPass VulkanCommandBuffer::GetCurrentRenderPass() const
	{
		return _currentRenderPass.renderPass;
	}

	VkFramebuffer VulkanCommandBuffer::GetCurrentFramebuffer() const
	{
		return _currentRenderPass.framebuffer;
	}
}