#include "VulkanUniformBuffer.h"
#include "D3D11UniformBuffer.h"
#include "UniformBuffer.h"

namespace SunEngine
{

	UniformBuffer::UniformBuffer() : GraphicsObject(GraphicsObject::UNIFORM_BUFFER)
	{
		_apiBuffer = 0;
		_size = 0;
	}


	UniformBuffer::~UniformBuffer()
	{
	}

	bool UniformBuffer::Create(const CreateInfo& info)
	{
		if (!Destroy())
			return false;

		if(!_apiBuffer)
			_apiBuffer = AllocateGraphics<IUniformBuffer>();

		IUniformBufferCreateInfo apiInfo = {};
		apiInfo.size = info.size;
		apiInfo.isShared = info.isShared;

		if (!_apiBuffer->Create(apiInfo)) return false;

		_size = info.size;
		return true;
	}

	bool UniformBuffer::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_apiBuffer = 0;
		return true;
	}

	bool UniformBuffer::Update(const void * pData)
	{
		return _apiBuffer->Update(pData);
	}

	bool UniformBuffer::Update(const void* pData, uint offset, uint size)
	{
		return _apiBuffer->Update(pData, offset, size);
	}

	bool UniformBuffer::UpdateShared(const void* pData, uint numElements)
	{
		return _apiBuffer->UpdateShared(pData, numElements);
	}

	uint UniformBuffer::GetMaxSharedUpdates() const
	{
		return _apiBuffer->GetMaxSharedUpdates();
	}

	uint UniformBuffer::GetSize() const
	{
		return _size;
	}

	IObject * UniformBuffer::GetAPIHandle() const
	{
		return _apiBuffer;
	}

	IUniformBuffer* IUniformBuffer::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanUniformBuffer();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11UniformBuffer();
		default:
			return 0;
		}
	}

}