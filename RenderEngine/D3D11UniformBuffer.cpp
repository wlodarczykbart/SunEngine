#include "D3D11Shader.h"
#include "D3D11CommandBuffer.h"
#include "D3D11UniformBuffer.h"

//https://docs.microsoft.com/en-us/windows/win32/api/d3d11_1/nf-d3d11_1-id3d11devicecontext1-vssetconstantbuffers1

#define CONST_BUFFER_ELEM_SIZE 16
#define CONST_BUFFER_MIN_ALIGNMENT (CONST_BUFFER_ELEM_SIZE * 16)

namespace SunEngine
{

	D3D11UniformBuffer::D3D11UniformBuffer()
	{
	}


	D3D11UniformBuffer::~D3D11UniformBuffer()
	{
	}

	bool D3D11UniformBuffer::Create(const IUniformBufferCreateInfo& info)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER;
		desc.ByteWidth = info.isShared ? D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * CONST_BUFFER_ELEM_SIZE : info.size;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_DYNAMIC;

		if (!_device->CreateBuffer(desc, 0, &_buffer)) return false;

		_size = info.size;
		_allocSize = desc.ByteWidth;
		return true;
	}

	bool D3D11UniformBuffer::Destroy()
	{
		COM_RELEASE(_buffer);
		return true;
	}

	bool D3D11UniformBuffer::Update(const void* pData)
	{
		return Update(pData, 0, _size);
	}

	bool D3D11UniformBuffer::Update(const void* pData, uint offset, uint size)
	{
		if (size + offset > _allocSize)
			return false;

		D3D11_MAPPED_SUBRESOURCE mappedData;
		if (!_device->Map(_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData)) return false;
		{
			memcpy((void*)((usize)mappedData.pData + offset), pData, size);
		}
		if (!_device->Unmap(_buffer, 0)) return false;

		return true;
	}

	bool D3D11UniformBuffer::UpdateShared(const void* pData, uint numElements)
	{
		//don't call this on non shared buffers
		if (_allocSize == _size)
			return false;

		uint alignedSize = GetAlignedSize();
		if (alignedSize * numElements > _allocSize)
			return false;

		D3D11_MAPPED_SUBRESOURCE mappedData;
		if (!_device->Map(_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData)) return false;

		usize dstMemAddr = (usize)mappedData.pData;
		usize srcMemAddr = (usize)pData;
		for (uint i = 0; i < numElements; i++)
		{
			memcpy((void*)dstMemAddr, (void*)srcMemAddr, _size); //offset was taken into account in map function
			dstMemAddr += alignedSize;
			srcMemAddr += _size;
		}

		if (!_device->Unmap(_buffer, 0)) return false;

		return true;
	}

	void D3D11UniformBuffer::BindToShader(D3D11Shader* pShader, const String& name, uint binding, IBindState* pBindState)
	{
		uint offset = 0;
		if (pBindState && pBindState->GetType() == IOBT_SHADER_BINDINGS)
		{
			IShaderBindingsBindState* state = static_cast<IShaderBindingsBindState*>(pBindState);
			uint idx = 0;
			while (!state->DynamicIndices[idx].first.empty() && idx < ARRAYSIZE(state->DynamicIndices))
			{
				if(state->DynamicIndices[idx].first == name)
				{
					offset = GetAlignedSize() * state->DynamicIndices[idx].second;
					break;
				}
				idx++;
			}		
		}
		pShader->BindBuffer(this, binding, offset / CONST_BUFFER_ELEM_SIZE, GetAlignedSize() / CONST_BUFFER_ELEM_SIZE);
	}

	void D3D11UniformBuffer::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void D3D11UniformBuffer::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	uint D3D11UniformBuffer::GetAlignedSize() const
	{
		return _size <= CONST_BUFFER_MIN_ALIGNMENT ? CONST_BUFFER_MIN_ALIGNMENT : (uint)ceilf((float)(_size) / CONST_BUFFER_MIN_ALIGNMENT) * CONST_BUFFER_MIN_ALIGNMENT;
	}

	uint D3D11UniformBuffer::GetMaxSharedUpdates() const
	{
		return _allocSize / GetAlignedSize();
	}
}