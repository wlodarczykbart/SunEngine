#pragma once

#include "IObject.h"

namespace SunEngine
{
	class ICommandBuffer
	{
	public:
		ICommandBuffer();
		virtual ~ICommandBuffer();

		virtual void Create() = 0;
		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Submit() = 0;

		virtual void Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance) = 0;
		virtual void DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance) = 0;
		virtual void SetScissor(float x, float y, float width, float height) = 0;
		virtual void SetViewport(float x, float y, float width, float height) = 0;

		static ICommandBuffer* Allocate(GraphicsAPI api);
	};

}