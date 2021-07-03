#include "D3D11CommandBuffer.h"
#include "D3D11RenderTarget.h"
#include "D3D11Texture.h"

namespace SunEngine
{
	static const Map<D3D11_SRV_DIMENSION, D3D11_RTV_DIMENSION> RTVDimensionMap =
	{
		{ D3D11_SRV_DIMENSION_TEXTURE2D, D3D11_RTV_DIMENSION_TEXTURE2D },
		{ D3D11_SRV_DIMENSION_TEXTURE2DARRAY, D3D11_RTV_DIMENSION_TEXTURE2DARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURE2DMS, D3D11_RTV_DIMENSION_TEXTURE2DMS },
		{ D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY, D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURECUBE,  D3D11_RTV_DIMENSION_TEXTURE2DARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURECUBEARRAY,  D3D11_RTV_DIMENSION_TEXTURE2DARRAY },
	};

	static const Map<D3D11_SRV_DIMENSION, D3D11_DSV_DIMENSION> DSVDimensionMap =
	{
		{ D3D11_SRV_DIMENSION_TEXTURE2D, D3D11_DSV_DIMENSION_TEXTURE2D },
		{ D3D11_SRV_DIMENSION_TEXTURE2DARRAY, D3D11_DSV_DIMENSION_TEXTURE2DARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURE2DMS, D3D11_DSV_DIMENSION_TEXTURE2DMS },
		{ D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY, D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURECUBE,  D3D11_DSV_DIMENSION_TEXTURE2DARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURECUBEARRAY,  D3D11_DSV_DIMENSION_TEXTURE2DARRAY },
	};

	D3D11RenderTarget::D3D11RenderTarget()
	{
		_numTargets = 0;
		_width = 0;
		_height = 0;
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
		_layers.resize(info.numLayers);
		if (!createViews(info.colorBuffers, info.depthBuffer)) 
			return false;

		return true;
	}

	bool D3D11RenderTarget::Destroy()
	{
		for (uint i = 0; i < _layers.size(); i++)
		{
			for (uint j = 0; j < _numTargets; j++)
				COM_RELEASE(_layers[i].rtv[j]);
			COM_RELEASE(_layers[i].dsv);
		}
		_layers.clear();

		_numTargets = 0;
		return true;
	}

	void D3D11RenderTarget::Bind(ICommandBuffer * cmdBuffer, IBindState* pBindState)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;

		bool clearOnBind = true;
		float clearColor[4] = { 1.0f, 0.5f, 0.5f, 1.0f };
		uint layer = 0;

		if (pBindState)
		{
			IRenderTargetBindState* state = static_cast<IRenderTargetBindState*>(pBindState);
			clearOnBind = state->clearOnBind;
			memcpy(clearColor, state->clearColor, sizeof(state->clearColor));
			layer = state->layer;
		}

		RenderLayer* pLayer = &_layers[layer];

		dxCmd->BindRenderTargets(_numTargets, pLayer->rtv, pLayer->dsv);

		if (_numTargets && clearOnBind)
		{
			for(uint i = 0; i < _numTargets; i++)
				dxCmd->ClearRenderTargetView(pLayer->rtv[i], clearColor);
		}

		if (pLayer->dsv && clearColor)
		{
			dxCmd->ClearDepthStencilView(pLayer->dsv, D3D11_CLEAR_DEPTH, 1.0f, 0xff);
		}

		dxCmd->SetViewport(0.0f, 0.0f, (float)_width, (float)_height);
		dxCmd->SetScissor(0.0f, 0.0f, (float)_width, (float)_height);
	}

	void D3D11RenderTarget::Unbind(ICommandBuffer * cmdBuffer)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;

		ID3D11DepthStencilView* nullDSV = 0;
		ID3D11RenderTargetView* nullRTV[MAX_SUPPORTED_RENDER_TARGETS] = {};

		dxCmd->BindRenderTargets(_numTargets, nullRTV, nullDSV);
	}

	bool D3D11RenderTarget::createViews(ITexture*const *pColorTexures, ITexture* pDepthTex)
	{
		for (uint i = 0; i < _layers.size(); i++)
		{
			for (uint j = 0; j < _numTargets; j++)
			{
				//DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
				D3D11Texture* pTex = static_cast<D3D11Texture*>(pColorTexures[j]);

				//_device->Get
				D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.ViewDimension = RTVDimensionMap.at(pTex->GetViewDimension());
				rtvDesc.Format = pTex->GetFormat();

				switch (rtvDesc.ViewDimension)
				{
				case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
					rtvDesc.Texture2DArray.ArraySize = 1;
					rtvDesc.Texture2DArray.FirstArraySlice = i;
					break;
				case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
					rtvDesc.Texture2DMSArray.ArraySize = 1;
					rtvDesc.Texture2DMSArray.FirstArraySlice = i;
					break;
				default:
					break;
				}

				if (!_device->CreateRenderTargetView(rtvDesc, pTex->_texture, &_layers[i].rtv[j])) return false;
			}

			if (pDepthTex)
			{
				D3D11Texture* pTex = static_cast<D3D11Texture*>(pDepthTex);

				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.ViewDimension = DSVDimensionMap.at(pTex->GetViewDimension());

				switch (dsvDesc.ViewDimension)
				{
				case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
					dsvDesc.Texture2DArray.ArraySize = 1;
					dsvDesc.Texture2DArray.FirstArraySlice = i;
					break;
				case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:
					dsvDesc.Texture2DMSArray.ArraySize = 1;
					dsvDesc.Texture2DMSArray.FirstArraySlice = i;
					break;
				default:
					break;
				}

				DXGI_FORMAT format = pTex->GetFormat();
				if (format == DXGI_FORMAT_R32_TYPELESS) dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				else if (format == DXGI_FORMAT_R32_FLOAT) dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				else if (format == DXGI_FORMAT_R24G8_TYPELESS) dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

				if (!_device->CreateDepthStencilView(dsvDesc, pTex->_texture, &_layers[i].dsv)) return false;
			}
		}

		return true;
	}

	D3D11RenderTarget::RenderLayer::RenderLayer()
	{
		for (uint i = 0; i < MAX_SUPPORTED_RENDER_TARGETS; i++)
			rtv[i] = 0;
		dsv = 0;
	}

}