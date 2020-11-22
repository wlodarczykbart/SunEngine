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
		bool Update(const void* pData, uint offset, const uint size) override;

		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		struct SetData
		{
			VkBuffer buffer;
			VkDeviceMemory bufferMem;
			VkDescriptorSet set;
			uint prevOffset;
			uint currentOffset;
		};

		bool CreateSet();
		IShaderResource _resource;

		VulkanShader* _shader;
		Vector<SetData> _sets;
		uint _currentSet;

		uint _setIndex;
		uint _setBinding;

		VulkanCommandBuffer* _currCmdBuffer;
	};

}