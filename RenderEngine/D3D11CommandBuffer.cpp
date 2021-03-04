#include "GraphicsContext.h"
#include "D3D11CommandBuffer.h"

namespace SunEngine
{
	D3D11CommandBuffer::D3D11CommandBuffer() 
	{
		_context = 0;
		_currentShader = 0;
	}

	D3D11CommandBuffer::~D3D11CommandBuffer()
	{
		_context = 0;
	}

	void D3D11CommandBuffer::Create()
	{
		D3D11Device* pDev = static_cast<D3D11Device*>(GraphicsContext::GetDevice());
		pDev->AllocateCommandBuffer(this);
	}

	void D3D11CommandBuffer::Begin()
	{
		_currentShader = 0;
	}

	void D3D11CommandBuffer::End()
	{
		_currentShader = 0;
	}

	void D3D11CommandBuffer::Submit()
	{
		_currentShader = 0;
	}

	void D3D11CommandBuffer::BindRenderTargets(UINT numViews, ID3D11RenderTargetView* const * ppViews, ID3D11DepthStencilView* pDSV)
	{
		_context->OMSetRenderTargets(numViews, ppViews, pDSV);
	}

	void D3D11CommandBuffer::BindViewports(uint numViewports, D3D11_VIEWPORT * pViewports)
	{
		_context->RSSetViewports(numViewports, pViewports);
	}

	void D3D11CommandBuffer::BindVertexBuffer(ID3D11Buffer* pBuffer, UINT stride, UINT offset)
	{
		_context->IASetVertexBuffers(0, 1, &pBuffer, &stride, &offset);
	}

	void D3D11CommandBuffer::BindIndexBuffer(ID3D11Buffer * pBuffer, DXGI_FORMAT indexFormat, uint offset)
	{
		_context->IASetIndexBuffer(pBuffer, indexFormat, offset);
	}

	void D3D11CommandBuffer::BindVertexShader(ID3D11VertexShader * pShader)
	{
		_context->VSSetShader(pShader, 0, 0);
	}

	void D3D11CommandBuffer::BindPixelShader(ID3D11PixelShader * pShader)
	{
		_context->PSSetShader(pShader, 0, 0);
	}

	void D3D11CommandBuffer::BindGeometryShader(ID3D11GeometryShader * pShader)
	{
		_context->GSSetShader(pShader, 0, 0);
	}

	void D3D11CommandBuffer::BindInputLayout(ID3D11InputLayout * pLayout)
	{
		_context->IASetInputLayout(pLayout);
	}

	void D3D11CommandBuffer::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
	{
		_context->IASetPrimitiveTopology(topology);
	}

	void D3D11CommandBuffer::BindRasterizerState(ID3D11RasterizerState * pRasterizer)
	{
		_context->RSSetState(pRasterizer);
	}

	void D3D11CommandBuffer::BindDepthStencilState(ID3D11DepthStencilState * pDepthStencil)
	{
		_context->OMSetDepthStencilState(pDepthStencil, D3D11_DEFAULT_STENCIL_WRITE_MASK);
	}

	void D3D11CommandBuffer::BindBlendState(ID3D11BlendState * pBlendState)
	{
		_context->OMSetBlendState(pBlendState, NULL, 0xffffffff);
	}

	void D3D11CommandBuffer::ClearRenderTargetView(ID3D11RenderTargetView * pRTV, FLOAT * rgba)
	{
		_context->ClearRenderTargetView(pRTV, rgba);
	}

	void D3D11CommandBuffer::ClearDepthStencilView(ID3D11DepthStencilView * pDSV, UINT clearFlags, FLOAT depth, UINT8 stencil)
	{
		_context->ClearDepthStencilView(pDSV, clearFlags, depth, stencil);
	}

	void D3D11CommandBuffer::Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
	{
		if (instanceCount == 1)
		{
			_context->Draw(vertexCount, firstVertex);
		}
		else
		{
			_context->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
		}
	}

	void D3D11CommandBuffer::DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance)
	{
		if (instanceCount == 1)
		{
			_context->DrawIndexed(indexCount, firstIndex, vertexOffset);
		}
		else
		{
			_context->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		}
	}

	void D3D11CommandBuffer::SetScissor(float x, float y, float width, float height)
	{
		D3D11_RECT rect = {};
		rect.left = (LONG)x;
		rect.bottom = (LONG)(y + height);
		rect.top = (LONG)y;
		rect.right = (LONG)(x + width);
		_context->RSSetScissorRects(1, &rect);
	}

	void D3D11CommandBuffer::SetViewport(float x, float y, float width, float height)
	{
		D3D11_VIEWPORT vp = {};
		vp.MaxDepth = 1.0f; //TODO assume ok?
		vp.MinDepth = 0.0f; //TODO assume ok?
		vp.TopLeftX = x;
		vp.TopLeftY = y;
		vp.Width = width;
		vp.Height = height;
		_context->RSSetViewports(1, &vp);
	}

	void D3D11CommandBuffer::SetCurrentShader(D3D11Shader* pShader)
	{
		_currentShader = pShader;
	}

	D3D11Shader* D3D11CommandBuffer::GetCurrentShader() const
	{
		return _currentShader;
	}

}