#pragma once

#include "IMesh.h"
#include "DirectXObject.h"

namespace SunEngine
{

	class DirectXMesh : public DirectXObject, public IMesh
	{
	public:
		DirectXMesh();
		~DirectXMesh();

		bool Create(const IMeshCreateInfo &info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		bool CreateDynamicVertexBuffer(uint stride, uint size, const void* pVerts) override;
		bool UpdateVertices(uint offset, uint size, const void* pVert) override;

		bool CreateDynamicIndexBuffer(uint numIndices, const uint* pIndices) override;
		bool UpdateIndices(uint offset, uint numIndices, const uint* pIndices) override;

	private:
		ID3D11Buffer* _vertexBuffer;
		ID3D11Buffer* _indexBuffer;
		uint _vertexStride;
	};

}