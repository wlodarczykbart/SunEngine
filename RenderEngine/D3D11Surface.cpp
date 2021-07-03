#include "D3D11CommandBuffer.h"
#include "D3D11Surface.h"

namespace SunEngine
{

	D3D11Surface::D3D11Surface()
	{
		_rtv = 0;
		_dsv = 0;
		_swapchain = 0;
	}


	D3D11Surface::~D3D11Surface()
	{
		Destroy();
	}

	bool D3D11Surface::Create(GraphicsWindow * window)
	{
		_window = window;

		if (!createSwapchain()) return false;
		if (!createRenderTargetViews()) return false;
		if (!createDepthStencilView()) return false;
		

		return true;
	}

	bool D3D11Surface::Destroy()
	{
		COM_RELEASE(_rtv);
		COM_RELEASE(_dsv);
		COM_RELEASE(_swapchain);

		return true;
	}

	bool D3D11Surface::StartFrame(ICommandBuffer * cmdBuffer)
	{
		return true;
	}

	bool D3D11Surface::SubmitFrame(ICommandBuffer * cmdBuffer)
	{
		_swapchain->Present(1, 0);
		return true;
	}

	void D3D11Surface::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;

		float rgba[]
		{
			0.4f, 0.77f, 0.35f, 1,
		};

		dxCmd->BindRenderTargets(1, &_rtv, _dsv);
		dxCmd->ClearRenderTargetView(_rtv, rgba);
		dxCmd->ClearDepthStencilView(_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0xff);

		dxCmd->SetViewport(0.0f, 0.0f, (float)_window->Width(), (float)_window->Height());
		dxCmd->SetScissor(0.0f, 0.0f, (float)_window->Width(), (float)_window->Height());
	}

	void D3D11Surface::Unbind(ICommandBuffer * cmdBuffer)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;

		ID3D11DepthStencilView* nullDSV = 0;
		ID3D11RenderTargetView* nullRTV = 0;

		dxCmd->BindRenderTargets(1, &nullRTV, nullDSV);
	}

	uint D3D11Surface::GetBackBufferCount() const
	{
		return 1;
	}


	bool D3D11Surface::createSwapchain()
	{
		DXGI_SWAP_CHAIN_DESC desc = {};
		desc.BufferCount = 2;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.OutputWindow = _window->Handle();
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Windowed = TRUE;

		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.Width = _window->Width();
		desc.BufferDesc.Height = _window->Height();
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		if (!_device->CreateSwapchain(desc, &_swapchain)) return false;

		return true;
	}

	bool D3D11Surface::createRenderTargetViews()
	{
		ID3D11Texture2D* backBuffer;
		_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D*), (void**)&backBuffer);

		D3D11_RENDER_TARGET_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		if (!_device->CreateRenderTargetView(desc, backBuffer, &_rtv)) return false;

		COM_RELEASE(backBuffer);
		return true;
	}

	bool D3D11Surface::createDepthStencilView()
	{
		ID3D11Texture2D* depthTex;

		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.ArraySize = 1;
		textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		textureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		textureDesc.Width = _window->Width();
		textureDesc.Height = _window->Height();
		textureDesc.MipLevels = 1;
		textureDesc.SampleDesc.Count = 1;		
		if (!_device->CreateTexture2D(textureDesc, NULL, &depthTex)) return false;

		D3D11_DEPTH_STENCIL_VIEW_DESC desc = {};
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		if (!_device->CreateDepthStencilView(desc, depthTex, &_dsv)) return false;

		COM_RELEASE(depthTex);

		return true;
	}

}