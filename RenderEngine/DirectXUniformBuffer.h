#pragma once

#include "IUniformBuffer.h"
#include "DirectXObject.h"

namespace SunEngine
{
	class DirectXUniformBuffer : public DirectXObject, public IUniformBuffer
	{
	public:
		DirectXUniformBuffer();
		~DirectXUniformBuffer();

		bool Create(const IUniformBufferCreateInfo& info) override;
		bool Destroy() override;
		bool Update(const void* pData) override;
		bool Update(const void* pData, uint offset, uint size) override;

		uint GetSize() const;

	protected:
		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		friend class DirectXShader;
		ID3D11Buffer* _buffer;
		IShaderResource _resource;
	};

}