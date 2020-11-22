#include "IMesh.h"
#include "BaseMesh.h"

namespace SunEngine
{
	BaseMesh::BaseMesh() : GraphicsObject(GraphicsObject::MESH)
	{
		_iMesh = 0;

		_bDynamicVertices = false;
		_bDynamicIndices = false;
	}

	BaseMesh::~BaseMesh()
	{
		Destroy();
	}

	bool BaseMesh::Create(const CreateInfo& info)
	{
		IMeshCreateInfo apiInfo = {};
		apiInfo.numIndices = info.numIndices;
		apiInfo.pIndices = info.pIndices;
		apiInfo.numVerts = info.numVerts;
		apiInfo.pVerts = info.pVerts;
		apiInfo.vertexStride = info.vertexStride;

		if (!Destroy())
			return false;

		if (!_iMesh)
			_iMesh = AllocateGraphics<IMesh>();

		_vertexStride = info.vertexStride;
		_numVertices = info.numVerts;
		_numIndices = info.numIndices;

		return _iMesh->Create(apiInfo);
	}

	bool BaseMesh::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_iMesh = 0;
		return true;
	}

	IObject * BaseMesh::GetAPIHandle() const
	{
		return _iMesh;
	}

	bool BaseMesh::UpdateVertices(const void* pVerts, uint bufferSize)
	{
		if (!_iMesh)
			return false;

		if (!_bDynamicVertices)
		{
			uint numVertices = bufferSize / _vertexStride;

			if (numVertices > _numVertices) 
				_numVertices = numVertices;

			if (!_iMesh->CreateDynamicVertexBuffer(_vertexStride, _numVertices * _vertexStride, 0))
				return false;
			_bDynamicVertices = true;

			return _iMesh->UpdateVertices(0, numVertices * _vertexStride, pVerts);
		}
		else
		{
			uint numVertices = bufferSize / _vertexStride;
			if (numVertices > _numVertices)
			{
				_numVertices = numVertices;
				if (!_iMesh->CreateDynamicVertexBuffer(_vertexStride, _numVertices * _vertexStride, pVerts))
					return false;
			}

			return _iMesh->UpdateVertices(0, numVertices * _vertexStride, pVerts);
		}
	}

	bool BaseMesh::UpdateIndices(const uint* pIndices, uint numIndices)
	{
		if (!_iMesh)
			return false;

		if (!_bDynamicIndices)
		{
			_numIndices = numIndices;
			if (!_iMesh->CreateDynamicIndexBuffer(numIndices, pIndices))
				return false;
			_bDynamicIndices = true;

			return _iMesh->UpdateIndices(0, _numIndices, pIndices);
		}
		else
		{
			if (numIndices > _numIndices)
			{
				_numIndices = numIndices;
				if (!_iMesh->CreateDynamicIndexBuffer(numIndices, pIndices))
					return false;
			}

			return _iMesh->UpdateIndices(0, numIndices, pIndices);
		}
	}

	uint BaseMesh::GetNumVertices() const
	{
		return _numVertices;
	}

	uint BaseMesh::GetNumIndices() const
	{
		return _numIndices;
	}
}