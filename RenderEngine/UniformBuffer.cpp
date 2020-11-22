#include "VulkanUniformBuffer.h"
#include "DirectXUniformBuffer.h"
#include "Shader.h"

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
		apiInfo.pShader = info.pShader;
		apiInfo.resource = info.resource;

		if (!_apiBuffer->Create(apiInfo)) return false;

		_size = info.resource.size;
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
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXUniformBuffer();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanUniformBuffer();
		default:
			return 0;
		}
	}

}