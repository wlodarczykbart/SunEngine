#pragma once

#include "GraphicsAPIDef.h"
#include "GraphicsObject.h"

#define MESH_VERSION 1

namespace SunEngine
{
	class BaseMesh : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			const void* pVerts;
			uint numVerts;
			uint vertexStride;
			const uint* pIndices;
			uint numIndices;
		};

		BaseMesh();
		virtual ~BaseMesh();

		IObject* GetAPIHandle() const override;

		bool Create(const CreateInfo& info);
		bool Destroy() override;

		bool UpdateVertices(const void* pVerts, uint bufferSize);
		bool UpdateIndices(const uint* pIndices, uint numIndices);

		uint GetNumVertices() const;
		uint GetNumIndices() const;

	private:
		IMesh* _iMesh;
		uint _numVertices;
		uint _numIndices;
		uint _vertexStride;
		bool _bDynamicVertices;
		bool _bDynamicIndices;
	};

}
