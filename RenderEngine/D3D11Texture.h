#pragma once

#include "ITexture.h"
#include "D3D11ShaderResource.h"

namespace SunEngine
{

	class D3D11Texture : public D3D11ShaderResource, public ITexture
	{
	public:
		D3D11Texture();
		~D3D11Texture();

		bool Create(const ITextureCreateInfo& info) override;
		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
		void BindToShader(D3D11Shader* pShader, const String& name, uint binding, IBindState* pBindState) override;

		bool Destroy() override;

	private:
		friend class D3D11Shader;
		friend class D3D11RenderTarget;
		friend class D3D11VRInterface;

		ID3D11Texture2D* _texture;
		ID3D11ShaderResourceView* _srv;
	};

}