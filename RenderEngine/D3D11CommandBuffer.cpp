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

	void D3D11CommandBuffer::BindComputeShader(ID3D11ComputeShader* pShader)
	{
		_context->CSSetShader(pShader, 0, 0);
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

	void D3D11CommandBuffer::Dispatch(uint groupCountX, uint groupCountY, uint groupCountZ)
	{
		_context->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void D3D11CommandBuffer::SetConstantBuffer(uint stageFlags, UINT startSlot, ID3D11Buffer* pBuffer, uint firstConstant, uint numConstants)
	{
		if (stageFlags & SS_VERTEX)
			firstConstant == 0 ? _context->VSSetConstantBuffers(startSlot, 1, &pBuffer) : _context->VSSetConstantBuffers1(startSlot, 1, &pBuffer, &firstConstant, &numConstants);
		if (stageFlags & SS_PIXEL)
			firstConstant == 0 ? _context->PSSetConstantBuffers(startSlot, 1, &pBuffer) : _context->PSSetConstantBuffers1(startSlot, 1, &pBuffer, &firstConstant, &numConstants);
		if (stageFlags & SS_GEOMETRY)
			firstConstant == 0 ? _context->GSSetConstantBuffers(startSlot, 1, &pBuffer) : _context->GSSetConstantBuffers1(startSlot, 1, &pBuffer, &firstConstant, &numConstants);
		if (stageFlags & SS_COMPUTE)
			firstConstant == 0 ? _context->CSSetConstantBuffers(startSlot, 1, &pBuffer) : _context->CSSetConstantBuffers1(startSlot, 1, &pBuffer, &firstConstant, &numConstants);
	}

	void D3D11CommandBuffer::SetSampler(uint stageFlags, UINT startSlot, ID3D11SamplerState* pSampler)
	{
		if (stageFlags & SS_VERTEX)
			_context->VSSetSamplers(startSlot, 1, &pSampler);
		if (stageFlags & SS_PIXEL)
			_context->PSSetSamplers(startSlot, 1, &pSampler);
		if (stageFlags & SS_GEOMETRY)
			_context->GSSetSamplers(startSlot, 1, &pSampler);
		if (stageFlags & SS_COMPUTE)
			_context->CSSetSamplers(startSlot, 1, &pSampler);
	}

	void D3D11CommandBuffer::SetShaderResourceView(uint stageFlags, UINT startSlot, ID3D11ShaderResourceView* pResource)
	{
		if (stageFlags & SS_VERTEX)
			_context->VSSetShaderResources(startSlot, 1, &pResource);
		if (stageFlags & SS_PIXEL)
			_context->PSSetShaderResources(startSlot, 1, &pResource);
		if (stageFlags & SS_GEOMETRY)
			_context->GSSetShaderResources(startSlot, 1, &pResource);
		if (stageFlags & SS_COMPUTE)
			_context->CSSetShaderResources(startSlot, 1, &pResource);
	}

	void D3D11CommandBuffer::SetUnorderedAccessView(uint stageFlags, UINT startSlot, ID3D11UnorderedAccessView* pResource)
	{
		//other stages don't have a uav method
		if (stageFlags & SS_COMPUTE)
			_context->CSSetUnorderedAccessViews(startSlot, 1, &pResource, 0);
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