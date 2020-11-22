#pragma once

#include "ICommandBuffer.h"
#include "DirectXObject.h"

namespace SunEngine
{
	class DirectXShader;

	class DirectXCommandBuffer : public ICommandBuffer
	{
	public:
		DirectXCommandBuffer();
		~DirectXCommandBuffer();

		void Create() override;
		void Begin() override;
		void End() override;
		void Submit() override;

		void BindRenderTargets(UINT numViews, ID3D11RenderTargetView* const * ppViews, ID3D11DepthStencilView* pDSV);
		void BindViewports(uint numViewports, D3D11_VIEWPORT* pViewports);
		void BindVertexBuffer(ID3D11Buffer* pBuffer, UINT stride, UINT offset);
		void BindIndexBuffer(ID3D11Buffer* pBuffer, DXGI_FORMAT indexFormat, uint offset);
		void BindVertexShader(ID3D11VertexShader* pShader);
		void BindPixelShader(ID3D11PixelShader* pShader);
		void BindGeometryShader(ID3D11GeometryShader* pShader);
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

		void SetCurrentShader(DirectXShader* pShader);
		DirectXShader* GetCurrentShader() const;

	private:
		friend class DirectXDevice;
		ID3D11DeviceContext* _context;

		DirectXShader* _currentShader;
	};
}