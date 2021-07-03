#include "D3D11Device.h"

#include "D3D11Object.h"
#include "D3D11CommandBuffer.h"
#include "StringUtil.h"

namespace SunEngine
{
	D3D11Device::D3D11Device()
	{
		_device = 0;
		_context = 0;
	}

	D3D11Device::~D3D11Device()
	{
	}

	bool D3D11Device::Create(const IDeviceCreateInfo& info)
	{
		UINT flags = 0;

		if(info.debugEnabled)
			flags |= D3D11_CREATE_DEVICE_DEBUG;

		D3D_FEATURE_LEVEL features[1];
		features[0] = D3D_FEATURE_LEVEL_11_1;

		D3D_FEATURE_LEVEL outFeatureLevel;

		ID3D11Device* device = 0;
		ID3D11DeviceContext* context = 0;
	
		CheckDXResult(D3D11CreateDevice(
			0,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			flags,
			features,
			SE_ARR_SIZE(features),
			D3D11_SDK_VERSION,
			&device,
			&outFeatureLevel,
			&context
		));

		if (outFeatureLevel != D3D_FEATURE_LEVEL_11_1) return false;

		CheckDXResult(device->QueryInterface(__uuidof(ID3D11Device1), (void**)&_device));
		CheckDXResult(context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&_context));

		COM_RELEASE(device);
		COM_RELEASE(context);

		return true;
	}

	bool D3D11Device::Destroy()
	{
		COM_RELEASE(_context);
		COM_RELEASE(_device);
		return true;
	}

	bool D3D11Device::CreateSwapchain(DXGI_SWAP_CHAIN_DESC & desc, IDXGISwapChain ** ppHandle)
	{
		struct {
			IDXGIAdapter* adapter;
			IDXGIFactory* factory;
			IDXGIDevice*  device;
		} dxgi;

		CheckDXResult(_device->QueryInterface(__uuidof(IDXGIDevice*), (void**)&dxgi.device));
		CheckDXResult(dxgi.device->GetAdapter(&dxgi.adapter));
		CheckDXResult(dxgi.adapter->GetParent(__uuidof(IDXGIFactory*), (void**)&dxgi.factory));

		CheckDXResult(dxgi.factory->CreateSwapChain(_device, &desc, ppHandle));

		COM_RELEASE(dxgi.factory);
		COM_RELEASE(dxgi.adapter);
		COM_RELEASE(dxgi.device);

		return true;
	}

	bool D3D11Device::CreateRenderTargetView(D3D11_RENDER_TARGET_VIEW_DESC & desc, ID3D11Texture2D * pResource, ID3D11RenderTargetView ** ppHandle)
	{
		CheckDXResult(_device->CreateRenderTargetView(pResource, &desc, ppHandle));
		return true;
	}

	bool D3D11Device::CreateDepthStencilView(D3D11_DEPTH_STENCIL_VIEW_DESC & desc, ID3D11Texture2D * pResource, ID3D11DepthStencilView ** ppHandle)
	{
		CheckDXResult(_device->CreateDepthStencilView(pResource, &desc, ppHandle));
		return true;
	}

	bool D3D11Device::CreateBuffer(D3D11_BUFFER_DESC & desc, D3D11_SUBRESOURCE_DATA* pSubData, ID3D11Buffer ** ppHandle)
	{
		CheckDXResult(_device->CreateBuffer(&desc, pSubData, ppHandle));
		return true;
	}

	bool D3D11Device::CreateSampler(D3D11_SAMPLER_DESC & desc, ID3D11SamplerState ** ppHandle)
	{
		CheckDXResult(_device->CreateSamplerState(&desc, ppHandle));
		return true;
	}

	bool D3D11Device::CreateVertexShader(void * pCompiledCode, UINT codeSize, ID3D11VertexShader ** ppHandle)
	{
		CheckDXResult(_device->CreateVertexShader(pCompiledCode, codeSize, 0, ppHandle));
		return true;
	}

	bool D3D11Device::CreatePixelShader(void * pCompiledCode, UINT codeSize, ID3D11PixelShader ** ppHandle)
	{
		CheckDXResult(_device->CreatePixelShader(pCompiledCode, codeSize, 0, ppHandle));
		return true;
	}

	bool D3D11Device::CreateGeometryShader(void * pCompiledCode, UINT codeSize, ID3D11GeometryShader ** ppHandle)
	{
		CheckDXResult(_device->CreateGeometryShader(pCompiledCode, codeSize, 0, ppHandle));
		return true;
	}

	bool D3D11Device::CreateComputeShader(void* pCompiledCode, UINT codeSize, ID3D11ComputeShader** ppHandle)
	{
		CheckDXResult(_device->CreateComputeShader(pCompiledCode, codeSize, 0, ppHandle));
		return true;
	}

	bool D3D11Device::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* pElements, UINT numElements, void * pCompiledVertexShaderCode, UINT codeSize, ID3D11InputLayout ** ppHandle)
	{
		CheckDXResult(_device->CreateInputLayout(pElements, numElements, pCompiledVertexShaderCode, codeSize, ppHandle));
		return true;
	}

	bool D3D11Device::CreateTexture2D(D3D11_TEXTURE2D_DESC & desc, D3D11_SUBRESOURCE_DATA* subData, ID3D11Texture2D ** ppHandle)
	{
		CheckDXResult(_device->CreateTexture2D(&desc, subData, ppHandle));
		return true;
	}

	bool D3D11Device::CreateShaderResourceView(ID3D11Resource* pResource, D3D11_SHADER_RESOURCE_VIEW_DESC & desc, ID3D11ShaderResourceView ** ppHandle)
	{
		CheckDXResult(_device->CreateShaderResourceView(pResource, &desc, ppHandle));
		return true;
	}

	bool D3D11Device::CreateRasterizerState(D3D11_RASTERIZER_DESC & desc, ID3D11RasterizerState ** ppHandle)
	{
		CheckDXResult(_device->CreateRasterizerState(&desc, ppHandle));
		return true;
	}

	bool D3D11Device::CreateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& desc, ID3D11DepthStencilState** ppHandle)
	{
		CheckDXResult(_device->CreateDepthStencilState(&desc, ppHandle));
		return true;
	}

	bool D3D11Device::CreateBlendState(D3D11_BLEND_DESC& desc, ID3D11BlendState** ppHandle)
	{
		CheckDXResult(_device->CreateBlendState(&desc, ppHandle));
		return true;
	}

	bool D3D11Device::CreateUnorderedAccessView(ID3D11Resource* pResource, D3D11_UNORDERED_ACCESS_VIEW_DESC& desc, ID3D11UnorderedAccessView** ppHandle)
	{
		CheckDXResult(_device->CreateUnorderedAccessView(pResource, &desc, ppHandle));
		return true;
	}

	bool D3D11Device::Map(ID3D11Resource* pResource, UINT subresource, D3D11_MAP mapType, UINT mapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
	{
		CheckDXResult(_context->Map(pResource, subresource, mapType, mapFlags, pMappedResource));
		return true;
	}

	bool D3D11Device::Unmap(ID3D11Resource* pResource, UINT subresource)
	{
		_context->Unmap(pResource, subresource);
		return true;
	}

	bool D3D11Device::AllocateCommandBuffer(D3D11CommandBuffer * cmdBuffer)
	{
		cmdBuffer->_context = _context;
		return false;
	}

	bool D3D11Device::UpdateSubresource(ID3D11Resource* pResource, UINT dstSubresource, D3D11_BOX* pDstBox, const void* pSrcData, UINT srcPitchRow, UINT srcDepthPitch)
	{
		_context->UpdateSubresource(pResource, dstSubresource, pDstBox, pSrcData, srcPitchRow, srcDepthPitch);
		return true;
	}

	bool D3D11Device::GenerateMips(ID3D11ShaderResourceView *pResource)
	{
		_context->GenerateMips(pResource);
		return true;
	}

	bool D3D11Device::CopyResource(ID3D11Resource* pDst, ID3D11Resource* pSrc)
	{
		_context->CopyResource(pDst, pSrc);
		return true;
	}

	bool D3D11Device::FillSampleDesc(DXGI_FORMAT format, uint samples, DXGI_SAMPLE_DESC& desc)
	{
		uint qualityLevels;
		HRESULT result = _device->CheckMultisampleQualityLevels(format, samples, &qualityLevels);
		if (result == S_OK)
		{
			desc.Count = samples;
			desc.Quality = qualityLevels - 1;
			return true;
		}
		else
		{
			desc.Count = 1;
			desc.Quality = 0;
			return false;
		}
	}

	void D3D11Device::LogError(const String& err)
	{
		_errMsg = err;
	}

	const String &D3D11Device::GetErrorMsg() const
	{
		return _errMsg;
	}

	String D3D11Device::QueryAPIError()
	{
		String errStr;

		if (_device->GetCreationFlags() & D3D11_CREATE_DEVICE_DEBUG) 
		{
			ID3D11InfoQueue* pInfoQueue = 0;
			_device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue);
			UINT64 msg1 = pInfoQueue->GetNumStoredMessages();
			for (UINT64 msg = 0; msg < msg1; msg++)
			{
				// Get the size of the message
				SIZE_T messageLength = 0;
				HRESULT hr = pInfoQueue->GetMessage(msg, NULL, &messageLength);

				// Allocate space and get the message
				D3D11_MESSAGE* pMessage = (D3D11_MESSAGE*)malloc(messageLength);
				hr = pInfoQueue->GetMessage(msg, pMessage, &messageLength);

				if (
					pMessage->Severity == D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_ERROR ||
					pMessage->Severity == D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_CORRUPTION ||
					pMessage->Severity == D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_WARNING)
				{
					errStr += pMessage->pDescription;

					if (msg + 1 != msg1)
						errStr += "\n";
				}

				free(pMessage);
			}

			if (errStr.length())
				pInfoQueue->ClearStoredMessages();
			COM_RELEASE(pInfoQueue);
		}
		return errStr;
	}

	bool D3D11Device::WaitIdle()
	{
		return true;
	}
}