#pragma once

#include "ISampler.h"
#include "DirectXShaderResource.h"

#include "SamplerSettings.h"

namespace SunEngine
{
	class DirectXSampler : public DirectXShaderResource, public ISampler
	{
	public:
		DirectXSampler();
		~DirectXSampler();

		bool Create(const ISamplerCreateInfo &info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
		void BindToShader(DirectXShader* pShader, uint binding) override;

	private:
		friend class DirectXShader;

		ID3D11SamplerState* _sampler;
	};

}