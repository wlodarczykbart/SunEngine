#pragma once

#include "IShaderBindings.h"
#include "IShader.h"
#include "DirectXObject.h"

namespace SunEngine
{
	class DirectXRenderTarget;
	class DirectXSampler;
	class DirectXUniformBuffer;
	class DirectXTexture;
	class ConfigFile;
	class DirectXTextureCube;
	class DirectXTextureArray;
	class DirectXShaderResource;

	class DirectXShader;

	class DirectXShaderBindings : public DirectXObject, public IShaderBindings
	{
	public:

		DirectXShaderBindings();
		~DirectXShaderBindings();

		bool Create(IShader* pShader, const Vector<IShaderResource>& textureBindings, const Vector<IShaderResource>& samplerBindings) override;
		void SetTexture(ITexture* pTexture, uint binding) override;
		void SetSampler(ISampler* pSampler, uint binding) override;
		void SetTextureCube(ITextureCube* pTextureCube, uint binding) override;
		void SetTextureArray(ITextureArray* pTextureArray, uint binding) override;

		void Bind(ICommandBuffer *pCmdBuffer) override;
		void Unbind(ICommandBuffer *pCmdBuffer) override;

	private:
		Map<uint, DirectXShaderResource*> _textureBindings;
		Map<uint, DirectXShaderResource*> _samplerBindings;
	};

	class DirectXShader : public DirectXObject, public IShader
	{
	public:
		DirectXShader();
		~DirectXShader();

		bool Create(IShaderCreateInfo& info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		void BindTexture(DirectXTexture* pTexture, uint binding);
		void BindTextureCube(DirectXTextureCube* pTextureCube, uint binding);
		void BindTextureArray(DirectXTextureArray* pTextureArray, uint binding);
		void BindSampler(DirectXSampler* pSampler, uint binding);

		uint GetTextureType(uint binding) const;
	private:
		typedef void(SunEngine::DirectXShader::*BindConstantBufferFunc)(ID3D11Buffer*, uint binding);
		typedef void(SunEngine::DirectXShader::*BindSamplerFunc)(ID3D11SamplerState*, uint binding);
		typedef void(SunEngine::DirectXShader::*BindShaderResourceFunc)(ID3D11ShaderResourceView*, uint binding);

		void BindShaderResourceView(ID3D11ShaderResourceView* pSRV, uint binding);

		void BindConstantBufferVS(ID3D11Buffer* pBuffer, uint binding);
		void BindConstantBufferPS(ID3D11Buffer* pBuffer, uint binding);
		void BindConstantBufferGS(ID3D11Buffer* pBuffer, uint binding);

		void BindSamplerVS(ID3D11SamplerState* pSampler, uint binding);
		void BindSamplerPS(ID3D11SamplerState* pSampler, uint binding);
		void BindSamplerGS(ID3D11SamplerState* pSampler, uint binding);

		void BindShaderResourceVS(ID3D11ShaderResourceView* pShaderResourceView, uint binding);
		void BindShaderResourcePS(ID3D11ShaderResourceView* pShaderResourceView, uint binding);
		void BindShaderResourceGS(ID3D11ShaderResourceView* pShaderResourceView, uint binding);

		struct ConstBufferData
		{
			Vector<BindConstantBufferFunc> funcs;
			DirectXUniformBuffer* pBuffer;

			ConstBufferData();
		};

		friend class DirectXGraphicsPipeline;

		ID3D11VertexShader* _vertexShader;
		ID3D11PixelShader* _pixelShader;
		ID3D11GeometryShader* _geometryShader;
		ID3D11InputLayout* _inputLayout;

		Map<uint, Vector<BindConstantBufferFunc>> _constBufferFuncMap;
		Map<uint, Vector<BindSamplerFunc>> _samplerFuncMap;
		Map<uint, Vector<BindShaderResourceFunc>> _textureFuncMap;
		Map<uint, uint> _textureFlagsMap;
	
	};

}