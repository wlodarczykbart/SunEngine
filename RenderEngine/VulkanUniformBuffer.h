#pragma once

#include "IUniformBuffer.h"
#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanShader;
	class VulkanCommandBuffer;

	class VulkanUniformBuffer : public VulkanObject, public IUniformBuffer
	{
	public:
		VulkanUniformBuffer();
		~VulkanUniformBuffer();

		bool Create(const IUniformBufferCreateInfo& info) override;
		bool Destroy() override;

		bool Update(const void* pData) override;
		bool Update(const void* pData, uint offset, uint size) override;
		bool UpdateShared(const void* pData, uint numElements) override;

		uint GetAlignedSize() const;
		uint GetMaxSharedUpdates() const override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		struct BufferData
		{
			VkBuffer buffer;
			VkDeviceMemory memory;
		};

		friend class VulkanShaderBindings;
		const BufferData& GetCurrentBuffer() const;

		Vector<BufferData> _buffers;
		uint _size;
		uint _allocSize;
	};

}