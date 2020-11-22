#include "DirectXCommandBuffer.h"
#include "DirectXRenderTarget.h"
#include "DirectXTexture.h"

namespace SunEngine
{

	DirectXRenderTarget::DirectXRenderTarget()
	{
		_clearOnBind = true;
		_clearColor[0] = 0.0f;
		_clearColor[1] = 0.0f;
		_clearColor[2] = 0.0f;
		_clearColor[3] = 0.0f;

		_rtv = 0;
		_dsv = 0;
	}


	DirectXRenderTarget::~DirectXRenderTarget()
	{
		Destroy();
	}

	bool DirectXRenderTarget::Create(const IRenderTargetCreateInfo & info)
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

	bool DirectXRenderTarget::Destroy()
	{
		COM_RELEASE(_rtv);
		COM_RELEASE(_dsv);
		return true;
	}

	void DirectXRenderTarget::Bind(ICommandBuffer * cmdBuffer)
	{
		DirectXCommandBuffer* dxCmd = (DirectXCommandBuffer*)cmdBuffer;

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

	void DirectXRenderTarget::Unbind(ICommandBuffer * cmdBuffer)
	{
		DirectXCommandBuffer* dxCmd = (DirectXCommandBuffer*)cmdBuffer;

		ID3D11DepthStencilView* nullDSV = 0;
		ID3D11RenderTargetView* nullRTV = 0;

		dxCmd->BindRenderTargets(1, &nullRTV, nullDSV);
	}

	void DirectXRenderTarget::SetClearColor(const float r, const float g, const float b, const float a)
	{
		_clearColor[0] = r;
		_clearColor[1] = g;
		_clearColor[2] = b;
		_clearColor[3] = a;
	}

	void DirectXRenderTarget::SetClearOnBind(bool clear)
	{
		_clearOnBind = clear;
	}

	void DirectXRenderTarget::SetViewport(float x, float y, float width, float height)
	{
		_viewport.TopLeftX = x;
		_viewport.TopLeftY = y;
		_viewport.Width = width;
		_viewport.Height = height;
	}

	bool DirectXRenderTarget::createViews(ITexture* pColorTex, ITexture* pDepthTex)
	{
		if (pColorTex)
		{
			//DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			DirectXTexture* pTex = static_cast<DirectXTexture*>(pColorTex);

			//_device->Get
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Format = pTex->_format;		
			if (!_device->CreateRenderTargetView(rtvDesc, pTex->_texture, &_rtv)) return false;
		}

		if (pDepthTex)
		{
			DirectXTexture* pTex = static_cast<DirectXTexture*>(pDepthTex);

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			if (!_device->CreateDepthStencilView(dsvDesc, pTex->_texture, &_dsv)) return false;
		}

		return true;
	}

}