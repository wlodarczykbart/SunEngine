#pragma once

#include "ITextureArray.h"
#include "D3D11ShaderResource.h"

namespace SunEngine
{

	class D3D11TextureArray : public D3D11ShaderResource, public ITextureArray
	{
	public:
		D3D11TextureArray();
		~D3D11TextureArray();

		bool Create(const ITextureArrayCreateInfo& info) override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
		void BindToShader(D3D11Shader* pShader, const String& name, uint binding, IBindState* pBindState) override;

		bool Destroy() override;

	private:
		friend class D3D11Shader;

		ID3D11Texture2D* _texture;
		ID3D11ShaderResourceView* _srv;
	};


}