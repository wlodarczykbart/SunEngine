#pragma once

#include "IMesh.h"
#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanMesh : public VulkanObject, public IMesh
	{
	public:
		VulkanMesh();
		~VulkanMesh();

		bool Create(const IMeshCreateInfo &info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		bool CreateDynamicVertexBuffer(uint stride, uint size, const void* pVerts);
		bool UpdateVertices(uint offset, uint size, const void* pVerts);

		bool CreateDynamicIndexBuffer(uint numIndices, const uint* pIndices);
		bool UpdateIndices(uint offset, uint numIndices, const uint* pIndices);

	private:
		VkBuffer _vertexBuffer;
		VulkanDevice::MemoryHandle _vertexMem;

		VkBuffer _indexBuffer;
		VulkanDevice::MemoryHandle _indexMem;
	};

}