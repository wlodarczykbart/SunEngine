#pragma once

#include "ITexture.h"
#include "DirectXShaderResource.h"

namespace SunEngine
{

	class DirectXTexture : public DirectXShaderResource, public ITexture
	{
	public:
		DirectXTexture();
		~DirectXTexture();

		bool Create(const ITextureCreateInfo& info) override;
		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
		void BindToShader(DirectXShader* pShader, uint binding) override;

		bool Destroy() override;

	private:
		friend class DirectXShader;
		friend class DirectXRenderTarget;

		ID3D11Texture2D* _texture;
		ID3D11ShaderResourceView* _srv;
		DXGI_FORMAT _format;
	};

}