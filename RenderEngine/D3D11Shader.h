#pragma once

#include "IShaderBindings.h"
#include "IShader.h"
#include "D3D11Object.h"

namespace SunEngine
{
	class D3D11RenderTarget;
	class D3D11Sampler;
	class D3D11UniformBuffer;
	class D3D11Texture;
	class ConfigFile;
	class D3D11TextureCube;
	class D3D11TextureArray;
	class D3D11ShaderResource;

	class D3D11Shader;

	class D3D11ShaderBindings : public D3D11Object, public IShaderBindings
	{
	public:

		D3D11ShaderBindings();
		~D3D11ShaderBindings();

		bool Create(const IShaderBindingCreateInfo& createInfo) override;
		void SetTexture(ITexture* pTexture, const String& name) override;
		void SetSampler(ISampler* pSampler, const String& name) override;
		void SetTextureCube(ITextureCube* pTextureCube, const String& name) override;
		void SetTextureArray(ITextureArray* pTextureArray, const String& name) override;
		void SetUniformBuffer(IUniformBuffer* pBuffer, const String& name) override;

		void Bind(ICommandBuffer *pCmdBuffer, IBindState* pBindState = 0) override;
		void Unbind(ICommandBuffer *pCmdBuffer) override;

	private:
		StrMap<Pair<uint, D3D11ShaderResource*>> _bindingMap;
	};

	class D3D11Shader : public D3D11Object, public IShader
	{
	public:
		D3D11Shader();
		~D3D11Shader();

		bool Create(IShaderCreateInfo& info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		void BindTexture(D3D11Texture* pTexture, uint binding);
		void BindTextureCube(D3D11TextureCube* pTextureCube, uint binding);
		void BindTextureArray(D3D11TextureArray* pTextureArray, uint binding);
		void BindSampler(D3D11Sampler* pSampler, uint binding);
		void BindBuffer(D3D11UniformBuffer* pBuffer, uint binding, uint firstConstant, uint numConstants);
	private:
		typedef void(SunEngine::D3D11Shader::*BindConstantBufferFunc)(ID3D11Buffer*, uint binding, uint firstConstant, uint numConstants);
		typedef void(SunEngine::D3D11Shader::*BindSamplerFunc)(ID3D11SamplerState*, uint binding);
		typedef void(SunEngine::D3D11Shader::*BindShaderResourceFunc)(ID3D11ShaderResourceView*, uint binding);

		void BindShaderResourceView(ID3D11ShaderResourceView* pSRV, uint binding);

		void BindConstantBufferVS(ID3D11Buffer* pBuffer, uint binding, uint firstConstant, uint numConstants);
		void BindConstantBufferPS(ID3D11Buffer* pBuffer, uint binding, uint firstConstant, uint numConstants);
		void BindConstantBufferGS(ID3D11Buffer* pBuffer, uint binding, uint firstConstant, uint numConstants);

		void BindSamplerVS(ID3D11SamplerState* pSampler, uint binding);
		void BindSamplerPS(ID3D11SamplerState* pSampler, uint binding);
		void BindSamplerGS(ID3D11SamplerState* pSampler, uint binding);

		void BindShaderResourceVS(ID3D11ShaderResourceView* pShaderResourceView, uint binding);
		void BindShaderResourcePS(ID3D11ShaderResourceView* pShaderResourceView, uint binding);
		void BindShaderResourceGS(ID3D11ShaderResourceView* pShaderResourceView, uint binding);

		friend class D3D11GraphicsPipeline;

		ID3D11VertexShader* _vertexShader;
		ID3D11PixelShader* _pixelShader;
		ID3D11GeometryShader* _geometryShader;
		ID3D11InputLayout* _inputLayout;

		Map<uint, Vector<BindConstantBufferFunc>> _constBufferFuncMap;
		Map<uint, Vector<BindSamplerFunc>> _samplerFuncMap;
		Map<uint, Vector<BindShaderResourceFunc>> _textureFuncMap;
	
	};

}