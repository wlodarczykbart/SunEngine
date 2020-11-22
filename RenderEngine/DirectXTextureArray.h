#pragma once

#include "ITextureArray.h"
#include "DirectXShaderResource.h"

namespace SunEngine
{

	class DirectXTextureArray : public DirectXShaderResource, public ITextureArray
	{
	public:
		DirectXTextureArray();
		~DirectXTextureArray();

		bool Create(const ITextureArrayCreateInfo& info) override;

		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
		void BindToShader(DirectXShader* pShader, uint binding) override;

		bool Destroy() override;

	private:
		friend class DirectXShader;

		ID3D11Texture2D* _texture;
		ID3D11ShaderResourceView* _srv;
	};


}