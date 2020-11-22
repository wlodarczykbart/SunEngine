#include <assert.h>
#include "DirectXCommandBuffer.h"
#include "DirectXMesh.h"

namespace SunEngine
{

	DirectXMesh::DirectXMesh()
	{
		_vertexBuffer = 0;
		_indexBuffer = 0;
		_vertexStride = 0;
	}


	DirectXMesh::~DirectXMesh()
	{
		Destroy();
	}

	bool DirectXMesh::Create(const IMeshCreateInfo & info)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA subData = {};

		desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
		desc.ByteWidth = info.numVerts*info.vertexStride;
		desc.StructureByteStride = info.vertexStride;
		subData.pSysMem = info.pVerts;
		if (!_device->CreateBuffer(desc, &subData, &_vertexBuffer)) return false;

		desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER;
		desc.ByteWidth = info.numIndices*sizeof(uint);
		desc.StructureByteStride = sizeof(uint);
		subData.pSysMem = info.pIndices;
		if (!_device->CreateBuffer(desc, &subData, &_indexBuffer)) return false;

		_vertexStride = info.vertexStride;

		return true;
	}

	bool DirectXMesh::Destroy()
	{
		COM_RELEASE(_vertexBuffer);
		COM_RELEASE(_indexBuffer);

		return true;
	}

	void DirectXMesh::Bind(ICommandBuffer * cmdBuffer)
	{
		DirectXCommandBuffer* dxCmd = (DirectXCommandBuffer*)cmdBuffer;
		dxCmd->BindVertexBuffer(_vertexBuffer, _vertexStride, 0);
		dxCmd->BindIndexBuffer(_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	}

	void DirectXMesh::Unbind(ICommandBuffer * cmdBuffer)
	{
		DirectXCommandBuffer* dxCmd = (DirectXCommandBuffer*)cmdBuffer;
		dxCmd->BindVertexBuffer(0, _vertexStride, 0);
		dxCmd->BindIndexBuffer(0, DXGI_FORMAT_R32_UINT, 0);
	}

	bool DirectXMesh::CreateDynamicVertexBuffer(uint stride, uint size, const void* pVerts)
	{
		COM_RELEASE(_vertexBuffer);

		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.ByteWidth = size;
		desc.StructureByteStride = stride;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		
		if (!_device->CreateBuffer(desc, 0, &_vertexBuffer)) return false;

		if(pVerts)
			if (!UpdateVertices(0, size, pVerts)) return false;

		return true;
	}

	bool DirectXMesh::UpdateVertices(uint offset, uint size, const void* pVerts)
	{

		D3D11_MAPPED_SUBRESOURCE subres = {};
		if (!_device->Map(_vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres)) return false;
		usize addr = (usize)(subres.pData);
		memcpy((void*)(addr + offset), pVerts, size);
		if (!_device->Unmap(_vertexBuffer, 0)) return false;

		//static Pair<usize, uint> test[6];
		//static uint counter = 0;

		//assert(offset == 0);
		//test[counter].first = addr;
		//test[counter].second = size;
		//counter++;

		//if (counter == 6)
		//{
		//	for (uint i = 0; i < 6; i++)
		//	{
		//		Pair<usize, uint> p0 = test[i];
		//		for (uint j = i + 1; j < 6; j++)
		//		{
		//			Pair<usize, uint> p1 = test[j];
		//			if (p0.first > p1.first) 
		//			{
		//				usize delta = p0.first - p1.first;
		//				assert(delta > p1.second);
		//			}
		//			else
		//			{
		//				usize delta = p1.first - p0.first;
		//				assert(delta > p0.second);
		//			}
		//		}
		//	}

		//	counter = 0;
		//}


		return true;
	}

	bool DirectXMesh::CreateDynamicIndexBuffer(uint numIndices, const uint* pIndices)
	{
		COM_RELEASE(_indexBuffer);

		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.ByteWidth = sizeof(uint) * numIndices;
		desc.StructureByteStride = sizeof(uint);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		
		if (!_device->CreateBuffer(desc, 0, &_indexBuffer)) return false;

		if (!UpdateIndices(0, numIndices, pIndices)) return false;

		return true;
	}

	bool DirectXMesh::UpdateIndices(uint offset, uint numIndices, const uint* pIndices)
	{
		uint size = sizeof(uint) * numIndices;
		D3D11_MAPPED_SUBRESOURCE subres;
		if (!_device->Map(_indexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres)) return false;
		memcpy((void*)((usize)(subres.pData) + offset), pIndices, size);
		if (!_device->Unmap(_indexBuffer, 0)) return false;

		return true;
	}

}