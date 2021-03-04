#include "VulkanCommandBuffer.h"

#include "VulkanMesh.h"

namespace SunEngine
{

	VulkanMesh::VulkanMesh()
	{
	}


	VulkanMesh::~VulkanMesh()
	{
		
	}

	bool VulkanMesh::Create(const IMeshCreateInfo & info)
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.size = info.numVerts * info.vertexStride;
		if (!_device->CreateBuffer(bufferInfo, &_vertexBuffer)) return false;
		if (!_device->AllocBufferMemory(_vertexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_vertexMem)) return false;
		if (!_device->TransferBufferData(_vertexBuffer, info.pVerts, (uint)bufferInfo.size)) return false;
			

		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		bufferInfo.size = info.numIndices * sizeof(uint);
		if (!_device->CreateBuffer(bufferInfo, &_indexBuffer)) return false;
		if (!_device->AllocBufferMemory(_indexBuffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_indexMem)) return false;
		if (!_device->TransferBufferData(_indexBuffer, info.pIndices, (uint)bufferInfo.size)) return false;

		return true;
	}

	bool VulkanMesh::Destroy()
	{
		_device->FreeMemory(_vertexMem);
		_device->DestroyBuffer(_vertexBuffer);
		_device->FreeMemory(_indexMem);
		_device->DestroyBuffer(_indexBuffer);
		return true;
	}

	void VulkanMesh::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		VulkanCommandBuffer* vkCmd = static_cast<VulkanCommandBuffer*>(cmdBuffer);
		vkCmd->BindVertexBuffer(_vertexBuffer, 0);
		vkCmd->BindIndexBuffer(_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanMesh::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	bool VulkanMesh::CreateDynamicVertexBuffer(uint stride, uint size, const void* pVerts)
	{
		(void)stride;

		_device->FreeMemory(_vertexMem);
		_device->DestroyBuffer(_vertexBuffer);

		VkBufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		info.size = size;
		
		if (!_device->CreateBuffer(info, &_vertexBuffer)) return false;
		if (!_device->AllocBufferMemory(_vertexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &_vertexMem)) return false;

		if (pVerts)
		{
			void* pMem;
			if (!_device->MapMemory(_vertexMem, 0, size, 0, &pMem)) return false;
			memcpy(pMem, pVerts, size);
			if (!_device->UnmapMemory(_vertexMem)) return false;
		}

		return true;

	}

	bool VulkanMesh::UpdateVertices(uint offset, uint size, const void* pVerts)
	{
		void* pMem;
		if (!_device->MapMemory(_vertexMem, offset, size, 0, &pMem)) return false;
		memcpy(pMem, pVerts, size);
		if (!_device->UnmapMemory(_vertexMem)) return false;

		return true;
	}

	bool VulkanMesh::CreateDynamicIndexBuffer(uint numIndices, const uint * pIndices)
	{

		_device->FreeMemory(_indexMem);
		_device->DestroyBuffer(_indexBuffer);

		uint size = sizeof(uint) * numIndices;

		VkBufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		info.size = size;

		if (!_device->CreateBuffer(info, &_indexBuffer)) return false;
		if (!_device->AllocBufferMemory(_indexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &_indexMem)) return false;

		void* pMem;
		if (!_device->MapMemory(_indexMem, 0, size, 0, &pMem)) return false;
		memcpy(pMem, pIndices, size);
		if (!_device->UnmapMemory(_indexMem)) return false;

		return true;
	}

	bool VulkanMesh::UpdateIndices(uint offset, uint numIndices, const uint * pIndices)
	{
		uint size = sizeof(uint) * numIndices;

		void* pMem;
		if (!_device->MapMemory(_indexMem, offset, size, 0, &pMem)) return false;
		memcpy(pMem, pIndices, size);
		if (!_device->UnmapMemory(_indexMem)) return false;

		return true;
	}

}