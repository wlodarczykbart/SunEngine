#pragma once

#include "IUniformBuffer.h"
#include "D3D11ShaderResource.h"

namespace SunEngine
{
	class D3D11UniformBuffer : public D3D11ShaderResource, public IUniformBuffer
	{
	public:
		D3D11UniformBuffer();
		~D3D11UniformBuffer();

		bool Create(const IUniformBufferCreateInfo& info) override;
		bool Destroy() override;
		bool Update(const void* pData) override;
		bool Update(const void* pData, uint offset, uint size) override;
		bool UpdateShared(const void* pData, uint numElements) override;

		void BindToShader(D3D11CommandBuffer* cmdBuffer, D3D11Shader* pShader, const String& name, uint binding, IBindState* pBindState) override;

		uint GetAlignedSize() const;
		uint GetMaxSharedUpdates() const override;
	protected:
		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		friend class D3D11Shader;
		ID3D11Buffer* _buffer;
		uint _size;
		uint _allocSize;
	};

}