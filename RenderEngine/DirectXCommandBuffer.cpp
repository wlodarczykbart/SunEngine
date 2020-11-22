#include "GraphicsContext.h"
#include "DirectXCommandBuffer.h"

namespace SunEngine
{
	DirectXCommandBuffer::DirectXCommandBuffer() 
	{
		_context = 0;
		_currentShader = 0;
	}

	DirectXCommandBuffer::~DirectXCommandBuffer()
	{
		_context = 0;
	}

	void DirectXCommandBuffer::Create()
	{
		DirectXDevice* pDev = static_cast<DirectXDevice*>(GraphicsContext::GetDevice());
		pDev->AllocateCommandBuffer(this);
	}

	void DirectXCommandBuffer::Begin()
	{
		_currentShader = 0;
	}

	void DirectXCommandBuffer::End()
	{
		_currentShader = 0;
	}

	void DirectXCommandBuffer::Submit()
	{
		_currentShader = 0;
	}

	void DirectXCommandBuffer::BindRenderTargets(UINT numViews, ID3D11RenderTargetView* const * ppViews, ID3D11DepthStencilView* pDSV)
	{
		_context->OMSetRenderTargets(numViews, ppViews, pDSV);
	}

	void DirectXCommandBuffer::BindViewports(uint numViewports, D3D11_VIEWPORT * pViewports)
	{
		_context->RSSetViewports(numViewports, pViewports);
	}

	void DirectXCommandBuffer::BindVertexBuffer(ID3D11Buffer* pBuffer, UINT stride, UINT offset)
	{
		_context->IASetVertexBuffers(0, 1, &pBuffer, &stride, &offset);
	}

	void DirectXCommandBuffer::BindIndexBuffer(ID3D11Buffer * pBuffer, DXGI_FORMAT indexFormat, uint offset)
	{
		_context->IASetIndexBuffer(pBuffer, indexFormat, offset);
	}

	void DirectXCommandBuffer::BindVertexShader(ID3D11VertexShader * pShader)
	{
		_context->VSSetShader(pShader, 0, 0);
	}

	void DirectXCommandBuffer::BindPixelShader(ID3D11PixelShader * pShader)
	{
		_context->PSSetShader(pShader, 0, 0);
	}

	void DirectXCommandBuffer::BindGeometryShader(ID3D11GeometryShader * pShader)
	{
		_context->GSSetShader(pShader, 0, 0);
	}

	void DirectXCommandBuffer::BindInputLayout(ID3D11InputLayout * pLayout)
	{
		_context->IASetInputLayout(pLayout);
	}

	void DirectXCommandBuffer::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
	{
		_context->IASetPrimitiveTopology(topology);
	}

	void DirectXCommandBuffer::BindRasterizerState(ID3D11RasterizerState * pRasterizer)
	{
		_context->RSSetState(pRasterizer);
	}

	void DirectXCommandBuffer::BindDepthStencilState(ID3D11DepthStencilState * pDepthStencil)
	{
		_context->OMSetDepthStencilState(pDepthStencil, D3D11_DEFAULT_STENCIL_WRITE_MASK);
	}

	void DirectXCommandBuffer::BindBlendState(ID3D11BlendState * pBlendState)
	{
		_context->OMSetBlendState(pBlendState, NULL, 0xffffffff);
	}

	void DirectXCommandBuffer::ClearRenderTargetView(ID3D11RenderTargetView * pRTV, FLOAT * rgba)
	{
		_context->ClearRenderTargetView(pRTV, rgba);
	}

	void DirectXCommandBuffer::ClearDepthStencilView(ID3D11DepthStencilView * pDSV, UINT clearFlags, FLOAT depth, UINT8 stencil)
	{
		_context->ClearDepthStencilView(pDSV, clearFlags, depth, stencil);
	}

	void DirectXCommandBuffer::Draw(uint vertexCount, uint instanceCount, uint firstVertex, uint firstInstance)
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

	void DirectXCommandBuffer::DrawIndexed(uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset, uint firstInstance)
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

	void DirectXCommandBuffer::SetScissor(float x, float y, float width, float height)
	{
		D3D11_RECT rect = {};
		rect.left = (LONG)x;
		rect.bottom = (LONG)(y + height);
		rect.top = (LONG)y;
		rect.right = (LONG)(x + width);
		_context->RSSetScissorRects(1, &rect);
	}

	void DirectXCommandBuffer::SetViewport(float x, float y, float width, float height)
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

	void DirectXCommandBuffer::SetCurrentShader(DirectXShader* pShader)
	{
		_currentShader = pShader;
	}

	DirectXShader* DirectXCommandBuffer::GetCurrentShader() const
	{
		return _currentShader;
	}

}