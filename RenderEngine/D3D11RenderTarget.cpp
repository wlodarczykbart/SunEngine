#include "D3D11CommandBuffer.h"
#include "D3D11RenderTarget.h"
#include "D3D11Texture.h"

namespace SunEngine
{

	D3D11RenderTarget::D3D11RenderTarget()
	{
		_clearOnBind = true;
		_clearColor[0] = 0.0f;
		_clearColor[1] = 0.0f;
		_clearColor[2] = 0.0f;
		_clearColor[3] = 0.0f;

		_numTargets = 0;
		_dsv = 0;
		_width = 0;
		_height = 0;
		_viewport = {};

		for (uint i = 0; i < MAX_SUPPORTED_RENDER_TARGETS; i++)
			_rtv[i] = 0;
	}


	D3D11RenderTarget::~D3D11RenderTarget()
	{
		Destroy();
	}

	bool D3D11RenderTarget::Create(const IRenderTargetCreateInfo & info)
	{
		_numTargets = info.numTargets;
		_width = info.width;
		_height = info.height;
		if (!createViews(info.colorBuffers, info.depthBuffer)) 
			return false;

		_viewport = {};
		_viewport.Width = (float)_width;
		_viewport.Height = (float)_height;
		_viewport.MinDepth = 0.0f;
		_viewport.MaxDepth = 1.0f;

		return true;
	}

	bool D3D11RenderTarget::Destroy()
	{
		for(uint i = 0; i < _numTargets; i++)
			COM_RELEASE(_rtv[i]);
		COM_RELEASE(_dsv);

		_numTargets = 0;
		return true;
	}

	void D3D11RenderTarget::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;

		dxCmd->BindRenderTargets(_numTargets, _rtv, _dsv);

		if (_numTargets && _clearOnBind)
		{
			for(uint i = 0; i < _numTargets; i++)
				dxCmd->ClearRenderTargetView(_rtv[i], _clearColor);
		}

		if (_dsv && _clearOnBind)
		{
			dxCmd->ClearDepthStencilView(_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0xff);
		}

		dxCmd->BindViewports(1, &_viewport);

	}

	void D3D11RenderTarget::Unbind(ICommandBuffer * cmdBuffer)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;

		ID3D11DepthStencilView* nullDSV = 0;
		ID3D11RenderTargetView* nullRTV[MAX_SUPPORTED_RENDER_TARGETS] = {};

		dxCmd->BindRenderTargets(_numTargets, nullRTV, nullDSV);
	}

	void D3D11RenderTarget::SetClearColor(const float r, const float g, const float b, const float a)
	{
		_clearColor[0] = r;
		_clearColor[1] = g;
		_clearColor[2] = b;
		_clearColor[3] = a;
	}

	void D3D11RenderTarget::SetClearOnBind(bool clear)
	{
		_clearOnBind = clear;
	}

	void D3D11RenderTarget::SetViewport(float x, float y, float width, float height)
	{
		_viewport.TopLeftX = x;
		_viewport.TopLeftY = y;
		_viewport.Width = width;
		_viewport.Height = height;
	}

	bool D3D11RenderTarget::createViews(ITexture*const *pColorTexures, ITexture* pDepthTex)
	{
		for (uint i = 0; i < _numTargets; i++)
		{
			//DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			D3D11Texture* pTex = static_cast<D3D11Texture*>(pColorTexures[i]);

			D3D11_TEXTURE2D_DESC texDesc;
			pTex->_texture->GetDesc(&texDesc);

			D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
			pTex->_srv->GetDesc(&viewDesc);

			//_device->Get
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.ViewDimension = texDesc.SampleDesc.Count == 1 ? D3D11_RTV_DIMENSION_TEXTURE2D : D3D11_RTV_DIMENSION_TEXTURE2DMS;
			rtvDesc.Format = viewDesc.Format;
			if (!_device->CreateRenderTargetView(rtvDesc, pTex->_texture, &_rtv[i])) return false;
		}

		if (pDepthTex)
		{
			D3D11Texture* pTex = static_cast<D3D11Texture*>(pDepthTex);

			D3D11_TEXTURE2D_DESC texDesc;
			pTex->_texture->GetDesc(&texDesc);

			D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
			pTex->_srv->GetDesc(&viewDesc);

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.ViewDimension = texDesc.SampleDesc.Count == 1 ? D3D11_DSV_DIMENSION_TEXTURE2D : D3D11_DSV_DIMENSION_TEXTURE2DMS;

			if (viewDesc.Format == DXGI_FORMAT_R32_TYPELESS) dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			else if (viewDesc.Format == DXGI_FORMAT_R32_FLOAT) dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			else if (viewDesc.Format == DXGI_FORMAT_R24G8_TYPELESS) dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

			if (!_device->CreateDepthStencilView(dsvDesc, pTex->_texture, &_dsv)) return false;
		}

		return true;
	}

}