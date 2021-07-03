#pragma once

#include "ICommandBuffer.h"
#include "D3D11Object.h"

namespace SunEngine
{
	class D3D11Shader;

	class D3D11CommandBuffer : public ICommandBuffer
	{
	public:
		D3D11CommandBuffer();
		~D3D11CommandBuffer();

		void Create() override;
		void Begin() override;
		void End() override;
		void Submit() override;

		void BindRenderTargets(UINT numViews, ID3D11RenderTargetView* const * ppViews, ID3D11DepthStencilView* pDSV);
		void BindVertexBuffer(ID3D11Buffer* pBuffer, UINT stride, UINT offset);
		void BindIndexBuffer(ID3D11Buffer* pBuffer, DXGI_FORMAT indexFormat, uint offset);
		void BindVertexShader(ID3D11VertexShader* pShader);
		void BindPixelShader(ID3D11PixelShader* pShader);
		void BindGeometryShader(ID3D11GeometryShader* pShader);
		void BindComputeShader(ID3D11ComputeShader* pShader);
		void BindInputLayout(ID3D11InputLayout* pLayout);
		void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);
		void BindRasterizerState(ID3D11RasterizerState* pRasterizer);
		void BindDepthStencilState(ID3D11DepthStencilState * pDepthStencil);
		void BindBlendState(ID3D11BlendState * pBlendState);

		void ClearRenderTargetView(ID3D11RenderTargetView* pRTV, FLOAT* rgba);
		void ClearDepthStencilView(ID3D11DepthStencilView* pDSV, UINT clearFlags, FLOAT depth, UINT8 stencil);

		void Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance) override;
		void DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance) override;
		void SetScissor(float x, float y, float width, float height) override;
		void SetViewport(float x, float y, float width, float height) override;
		void Dispatch(uint groupCountX, uint groupCountY, uint groupCountZ) override;

		void SetConstantBuffer(uint stageFlags, UINT startSlot, ID3D11Buffer* pBuffer, uint firstConstant, uint numConstants);
		void SetSampler(uint stageFlags, UINT startSlot, ID3D11SamplerState* pSampler);
		void SetShaderResourceView(uint stageFlags, UINT startSlot, ID3D11ShaderResourceView* pResource);
		void SetUnorderedAccessView(uint stageFlags, UINT startSlot, ID3D11UnorderedAccessView* pResource);

		void SetCurrentShader(D3D11Shader* pShader);
		D3D11Shader* GetCurrentShader() const;

	private:
		friend class D3D11Device;
		ID3D11DeviceContext1* _context;

		D3D11Shader* _currentShader;
	};
}