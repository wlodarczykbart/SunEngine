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

		_rtv = 0;
		_dsv = 0;
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
		if (!createViews(info.colorBuffer, info.depthBuffer)) return false;

		_viewport = {};
		_viewport.Width = (float)_width;
		_viewport.Height = (float)_height;
		_viewport.MinDepth = 0.0f;
		_viewport.MaxDepth = 1.0f;

		return true;
	}

	bool D3D11RenderTarget::Destroy()
	{
		COM_RELEASE(_rtv);
		COM_RELEASE(_dsv);
		return true;
	}

	void D3D11RenderTarget::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;

		dxCmd->BindRenderTargets(1, &_rtv, _dsv);

		if (_rtv && _clearOnBind)
		{
			dxCmd->ClearRenderTargetView(_rtv, _clearColor);
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
		ID3D11RenderTargetView* nullRTV = 0;

		dxCmd->BindRenderTargets(1, &nullRTV, nullDSV);
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

	bool D3D11RenderTarget::createViews(ITexture* pColorTex, ITexture* pDepthTex)
	{
		if (pColorTex)
		{
			//DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			D3D11Texture* pTex = static_cast<D3D11Texture*>(pColorTex);

			//_device->Get
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = pTex->_format;		
			if (!_device->CreateRenderTargetView(rtvDesc, pTex->_texture, &_rtv)) return false;
		}

		if (pDepthTex)
		{
			D3D11Texture* pTex = static_cast<D3D11Texture*>(pDepthTex);

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			if (!_device->CreateDepthStencilView(dsvDesc, pTex->_texture, &_dsv)) return false;
		}

		return true;
	}

}