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
		void BindToShader(D3D11CommandBuffer* cmdBuffer, D3D11Shader* pShader, const String& name, uint binding, IBindState* pBindState) override;
		bool Destroy() override;

		D3D11_SRV_DIMENSION GetViewDimension() const { return _viewDesc.ViewDimension; }
		DXGI_FORMAT GetFormat() const { return _viewDesc.Format; }

	private:
		friend class D3D11Shader;
		friend class D3D11RenderTarget;
		friend class D3D11VRInterface;

		ID3D11Texture2D* _texture;
		ID3D11ShaderResourceView* _srv;
		ID3D11ShaderResourceView* _cubeToArraySRV;
		ID3D11UnorderedAccessView* _uav;
		D3D11_SHADER_RESOURCE_VIEW_DESC _viewDesc;
		bool _external;
	};

}