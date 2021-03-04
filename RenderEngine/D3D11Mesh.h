#pragma once

#include "IMesh.h"
#include "D3D11Object.h"

namespace SunEngine
{

	class D3D11Mesh : public D3D11Object, public IMesh
	{
	public:
		D3D11Mesh();
		~D3D11Mesh();

		bool Create(const IMeshCreateInfo &info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
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