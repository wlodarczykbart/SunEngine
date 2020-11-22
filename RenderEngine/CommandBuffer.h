#pragma once

#include "GraphicsAPIDef.h"

namespace SunEngine
{
	class CommandBuffer
	{
	public:
		CommandBuffer();
		~CommandBuffer();

		void Create();
		void Destroy();

		void Begin();
		void End();
		void Submit();

		ICommandBuffer * GetAPIHandle() const;

		void DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance);
		void Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance);
		void SetScissor(float x, float y, float width, float height);
		void SetViewport(float x, float y, float width, float height);

	private:
		ICommandBuffer* _apiCmdBuffer;
	};

}