#include "DirectXShader.h"
#include "DirectXCommandBuffer.h"
#include "DirectXUniformBuffer.h"

namespace SunEngine
{

	DirectXUniformBuffer::DirectXUniformBuffer()
	{
	}


	DirectXUniformBuffer::~DirectXUniformBuffer()
	{
	}

	bool DirectXUniformBuffer::Create(const IUniformBufferCreateInfo& info)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = info.resource.size;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		
		if (!_device->CreateBuffer(desc, 0, &_buffer)) return false;

		_resource = info.resource;
		return true;
	}

	bool DirectXUniformBuffer::Destroy()
	{
		COM_RELEASE(_buffer);
		return true;
	}

	bool DirectXUniformBuffer::Update(const void * pData)
	{
		return Update(pData, 0, _resource.size);
	}

	bool DirectXUniformBuffer::Update(const void * pData, uint offset, uint size)
	{
		D3D11_MAPPED_SUBRESOURCE mappedData;
		if (!_device->Map(_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData)) return false;
		{
			memcpy((void*)((usize)mappedData.pData + offset), pData, size);
		}
		if (!_device->Unmap(_buffer, 0)) return false;

		return true;
	}

	void DirectXUniformBuffer::Bind(ICommandBuffer * cmdBuffer)
	{
		//DirectXCommandBuffer* dxCmd = static_cast<DirectXCommandBuffer*>(cmdBuffer);
		(void)cmdBuffer;

		if (_resource.stages & SS_VERTEX)
		{
			_device->VSSetConstantBuffers(_resource.binding, 1, &_buffer);
		}
		if (_resource.stages & SS_PIXEL)
		{
			_device->PSSetConstantBuffers(_resource.binding, 1, &_buffer);
		}
		if (_resource.stages & SS_GEOMETRY)
		{
			_device->GSSetConstantBuffers(_resource.binding, 1, &_buffer);
		}
	}

	void DirectXUniformBuffer::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}
	
	uint DirectXUniformBuffer::GetSize() const
	{
		return _resource.size;
	}

}