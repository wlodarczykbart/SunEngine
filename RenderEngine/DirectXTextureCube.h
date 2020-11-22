#pragma once

#include "ITextureCube.h"
#include "DirectXShaderResource.h"

namespace SunEngine
{

	class DirectXTextureCube : public DirectXShaderResource, public ITextureCube
	{
	public:
		DirectXTextureCube();
		~DirectXTextureCube();

		bool Create(const ITextureCubeCreateInfo& info) override;

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