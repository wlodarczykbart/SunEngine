#include "VulkanShader.h"
#include "VulkanCommandBuffer.h"
#include "VulkanUniformBuffer.h"


namespace SunEngine
{

	VulkanUniformBuffer::VulkanUniformBuffer()
	{
		_shader = 0;
		_currentSet = 0;
		_currCmdBuffer = 0;
	}


	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
	}

	bool VulkanUniformBuffer::Create(const IUniformBufferCreateInfo& info)
	{
		if (info.pShader == 0)
			return false;
		
		if (info.resource.size == 0)
			return false;

		_resource = info.resource;
		_shader = static_cast<VulkanShader*>(info.pShader);
		if (!CreateSet())
			return false;

		return true;
	}

	bool VulkanUniformBuffer::Destroy()
	{
		for (uint i = 0; i < _sets.size(); i++)
		{
			SetData& data = _sets[i];

			_device->FreeMemory(data.bufferMem);
			_device->DestroyBuffer(data.buffer);
			_device->FreeDescriptorSet(data.set);
		}


		return true;
	}

	bool VulkanUniformBuffer::Update(const void * pData)
	{
		return Update(pData, 0, _resource.size);
	}

	bool VulkanUniformBuffer::Update(const void * pData, const uint offset, const uint size)
	{
		SetData& data = _sets[_currentSet];

		void* deviceMem = 0;
		if (!(_device->MapMemory(data.bufferMem, data.currentOffset, _resource.size, 0, &deviceMem))) return false;	
		memcpy((void*)((usize)deviceMem + offset), pData, size); //offset was taken into account in map function
		if (!_device->UnmapMemory(data.bufferMem)) return false;

		uint remainder = (_resource.size - 1) / (_device->GetMinUniformBufferAlignment());
		uint alignedOffset = 1 * (_device->GetMinUniformBufferAlignment() << remainder);

		data.prevOffset = data.currentOffset;

		if (_currCmdBuffer)
		{
			if (_setIndex != VulkanShader::ENGINE_BUFFERS)
			{
				_currCmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _shader->GetPipelineLayout(), _setIndex, 1, &data.set, 1, &data.prevOffset);
			}
			else
			{
				uint* pOffsets, numOffsets;
				_shader->GetSharedBufferDynamicOffsets(&pOffsets, numOffsets);
				pOffsets[_setBinding] = data.prevOffset;
				_currCmdBuffer->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _shader->GetPipelineLayout(), _setIndex, 1, &data.set, numOffsets, pOffsets);
			}
		}

		data.currentOffset += alignedOffset;
		if (data.currentOffset + alignedOffset > _device->GetUniformBufferMaxSize())
		{
			_currentSet++;
			if (_currentSet == _sets.size())
			{
				int ww = 5;
				ww++;
			}
		}

		return true;
	}

	void VulkanUniformBuffer::Bind(ICommandBuffer * cmdBuffer)
	{
		_currCmdBuffer = static_cast<VulkanCommandBuffer*>(cmdBuffer);

		//this is the first time this ubo has been doung within a command buffer recording, we will reset the 
		//offsets and set to start from the beginning during the recording period, which ends upon CommandBuffer::Submit
		if (_currCmdBuffer->ReigsterUniformBuffer(this))
		{
			for (uint i = 0; i < _sets.size(); i++)
			{
				_sets[i].currentOffset = 0;
				_sets[i].prevOffset = 0;
			}

			_currentSet = 0;
		}
	}

	void VulkanUniformBuffer::Unbind(ICommandBuffer * cmdBuffer)
	{
		if (cmdBuffer == _currCmdBuffer)
			cmdBuffer = 0;
	}

	bool VulkanUniformBuffer::CreateSet()
	{
		VulkanShader::QuerySetAndBinding(SRT_CONST_BUFFER, _resource.binding, _setIndex, _setBinding);

		uint buffersToAllocate = _setIndex == VulkanShader::ENGINE_BUFFERS ? 1 : 20;

		for (uint i = 0; i < buffersToAllocate; i++)
		{
			SetData data = {};

			VkBufferCreateInfo vkInfo = {};
			vkInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vkInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			vkInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			vkInfo.size = _device->GetUniformBufferMaxSize();

			if (!_device->CreateBuffer(vkInfo, &data.buffer)) return false;
			if (!_device->AllocBufferMemory(data.buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &data.bufferMem)) return false;

			//this ubo will own its own descriptor set that only contains one binding per set...
			if (_setIndex != VulkanShader::ENGINE_BUFFERS)
			{
				VkDescriptorSetLayout setLayout = _shader->GetDescriptorSetLayout(_setIndex);

				VkDescriptorSetAllocateInfo setInfo = {};
				setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				setInfo.descriptorSetCount = 1;
				setInfo.pSetLayouts = &setLayout;

				if (!_device->AllocateDescriptorSets(setInfo, &data.set))
					return false;
			}
			else
			{
				data.set = _shader->GetSharedBufferDescriptorSet();
			}

			VkDescriptorBufferInfo info = {};
			info.buffer = data.buffer;
			info.offset = 0;
			info.range = _resource.size;

			VkWriteDescriptorSet set = {};
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.dstSet = data.set;
			set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			set.dstBinding = _setBinding;
			set.pBufferInfo = &info;
			set.descriptorCount = 1;

			if (!_device->UpdateDescriptorSets(&set, 1))
				return false;

			_sets.push_back(data);
		}
		return true;
	}

}