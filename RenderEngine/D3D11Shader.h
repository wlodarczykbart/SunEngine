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

		void BindTexture(D3D11CommandBuffer* cmdBuffer, D3D11Texture* pTexture, const String& name, uint binding);
		void BindSampler(D3D11CommandBuffer* cmdBuffer, D3D11Sampler* pSampler, const String& name, uint binding);
		void BindBuffer(D3D11CommandBuffer* cmdBuffer, D3D11UniformBuffer* pBuffer, const String& name, uint binding, uint firstConstant, uint numConstants);
	private:
		friend class D3D11GraphicsPipeline;

		ID3D11VertexShader* _vertexShader;
		ID3D11PixelShader* _pixelShader;
		ID3D11GeometryShader* _geometryShader;
		ID3D11ComputeShader* _computeShader;
		ID3D11InputLayout* _inputLayout;
		StrMap<IShaderResource> _resourceMap;
	
	};

}