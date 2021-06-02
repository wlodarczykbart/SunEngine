#include "VulkanShader.h"
#include "VulkanCommandBuffer.h"
#include "VulkanUniformBuffer.h"


namespace SunEngine
{

	VulkanUniformBuffer::VulkanUniformBuffer()
	{
		_size = 0;
		_allocSize = 0;
	}


	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
	}

	bool VulkanUniformBuffer::Create(const IUniformBufferCreateInfo& info)
	{
		VkBufferCreateInfo vkInfo = {};
		vkInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		vkInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		vkInfo.size = info.isShared ? _device->GetUniformBufferMaxSize() : info.size;
		_size = info.size;
		_allocSize = (uint)vkInfo.size;

		//any buffer that is shared is likely to be updated every frame and thus should be swapped between SHARED_BUFFER_FRAME_COUNT to make sure that subsequent frames don't override that data
		_buffers.resize(info.isShared ? VulkanDevice::BUFFERED_FRAME_COUNT : 1);

		for (uint i = 0; i < _buffers.size(); i++)
		{
			if (!_device->CreateBuffer(vkInfo, &_buffers[i].buffer)) return false;
			if (!_device->AllocBufferMemory(_buffers[i].buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &_buffers[i].memory)) return false;
		}

		return true;
	}

	bool VulkanUniformBuffer::Destroy()
	{
		return true;
	}

	bool VulkanUniformBuffer::Update(const void * pData)
	{
		return Update(pData, 0, _size);
	}

	bool VulkanUniformBuffer::Update(const void * pData, uint offset, uint size)
	{
		if (size + offset > _allocSize)
			return false;

		const BufferData& currBuffer = GetCurrentBuffer();

		void* deviceMem = 0;
		if (!(_device->MapMemory(currBuffer.memory, offset, _size, 0, &deviceMem))) return false;
		memcpy((void*)((usize)deviceMem), pData, size); //offset was taken into account in map function
		_device->UnmapMemory(currBuffer.memory);

		return true;
	}

	bool VulkanUniformBuffer::UpdateShared(const void* pData, uint numElements)
	{
		if (numElements == 0)
			return true;

		//don't call this on non shared buffers
		if (_allocSize == _size)
			return false;

		uint alignedSize = GetAlignedSize();
		if (alignedSize * numElements > _allocSize)
			return false;

		const BufferData& currBuffer = GetCurrentBuffer();

		void* deviceMem = 0;
		if (!(_device->MapMemory(currBuffer.memory, 0, alignedSize * numElements, 0, &deviceMem))) return false;

		usize dstMemAddr = (usize)deviceMem;
		usize srcMemAddr = (usize)pData;
		for(uint i = 0; i < numElements; i++)
		{
			memcpy((void*)dstMemAddr, (void*)srcMemAddr, _size); //offset was taken into account in map function
			dstMemAddr += alignedSize;
			srcMemAddr += _size;
		}

		_device->UnmapMemory(currBuffer.memory);;

		return true;
	}

	uint VulkanUniformBuffer::GetAlignedSize() const
	{
		uint minAlign = _device->GetMinUniformBufferAlignment();
		return _size <= minAlign ? minAlign : (uint)ceilf((float)(_size) / minAlign) * minAlign;
	}

	void VulkanUniformBuffer::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void VulkanUniformBuffer::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	const VulkanUniformBuffer::BufferData& VulkanUniformBuffer::GetCurrentBuffer() const
	{
		return _buffers.size() == 1 ? _buffers.front() : _buffers[_device->GetBufferedFrameNumber()];
	}

	uint VulkanUniformBuffer::GetMaxSharedUpdates() const
	{
		return _allocSize / GetAlignedSize();
	}

}