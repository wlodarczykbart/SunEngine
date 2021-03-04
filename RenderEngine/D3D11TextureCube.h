#pragma once

#include "ITextureCube.h"
#include "D3D11ShaderResource.h"

namespace SunEngine
{

	class D3D11TextureCube : public D3D11ShaderResource, public ITextureCube
	{
	public:
		D3D11TextureCube();
		~D3D11TextureCube();

		bool Create(const ITextureCubeCreateInfo& info) override;

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