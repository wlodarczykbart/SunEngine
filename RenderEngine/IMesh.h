#pragma once

#include "IObject.h"

namespace SunEngine
{
	class IMesh : public IObject
	{
	public:
		IMesh();
		virtual ~IMesh();

		virtual bool Create(const IMeshCreateInfo &info) = 0;
		 
		virtual bool CreateDynamicVertexBuffer(uint stride, uint size, const void* pVerts) = 0;
		virtual bool UpdateVertices(uint offset, uint size, const void* pVert) = 0;
		 
		virtual bool CreateDynamicIndexBuffer(uint numIndices, const uint* pIndices) = 0;
		virtual bool UpdateIndices(uint offset, uint numIndices, const uint* pIndices) = 0;

		static IMesh * Allocate(GraphicsAPI api);
	};
}