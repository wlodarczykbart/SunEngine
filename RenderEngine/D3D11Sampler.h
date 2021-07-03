#pragma once

#include "ISampler.h"
#include "D3D11ShaderResource.h"

#include "SamplerSettings.h"

namespace SunEngine
{
	class D3D11Sampler : public D3D11ShaderResource, public ISampler
	{
	public:
		D3D11Sampler();
		~D3D11Sampler();

		bool Create(const ISamplerCreateInfo &info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
		void BindToShader(D3D11CommandBuffer* cmdBuffer, D3D11Shader* pShader, const String& name, uint binding, IBindState* pBindState) override;

	private:
		friend class D3D11Shader;

		ID3D11SamplerState* _sampler;
	};

}