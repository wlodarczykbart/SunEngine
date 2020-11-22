#include "DirectXCommandBuffer.h"
#include "VulkanCommandBuffer.h"
#include "CommandBuffer.h"

namespace SunEngine
{

	CommandBuffer::CommandBuffer()
	{
		_apiCmdBuffer = 0;
	}


	CommandBuffer::~CommandBuffer()
	{
		Destroy();
	}

	void CommandBuffer::Create()
	{
		Destroy();

		_apiCmdBuffer = AllocateGraphics<ICommandBuffer>();
		_apiCmdBuffer->Create();
	}

	void CommandBuffer::Destroy()
	{
		if (_apiCmdBuffer)
		{
			delete _apiCmdBuffer;
			_apiCmdBuffer = 0;
		}
	}

	void CommandBuffer::Begin()
	{
		_apiCmdBuffer->Begin();
	}

	void CommandBuffer::End()
	{
		_apiCmdBuffer->End();
	}

	void CommandBuffer::Submit()
	{
		_apiCmdBuffer->Submit();
	}

	ICommandBuffer* CommandBuffer::GetAPIHandle() const
	{
		return _apiCmdBuffer;
	}

	void CommandBuffer::DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance)
	{
		_apiCmdBuffer->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void CommandBuffer::Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
	{
		_apiCmdBuffer->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void CommandBuffer::SetScissor(float x, float y, float width, float height)
	{
		_apiCmdBuffer->SetScissor(x, y, width, height);
	}

	void CommandBuffer::SetViewport(float x, float y, float width, float height)
	{
		_apiCmdBuffer->SetViewport(x, y, width, height);
	}

}