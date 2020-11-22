#include "DirectXDevice.h"

#include "DirectXObject.h"
#include "DirectXCommandBuffer.h"
#include "StringUtil.h"

#define CheckDXResult(expression) if(expression != S_OK) { _errMsg = StrFormat("Error on line %d, %s\n",  __LINE__, #expression); return false; }

namespace SunEngine
{
	DirectXDevice::DirectXDevice()
	{
		_device = 0;
	}

	DirectXDevice::~DirectXDevice()
	{
	}

	bool DirectXDevice::Create()
	{
		UINT flags = 0;

#ifdef _DEBUG 
		flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL features[1];
		features[0] = D3D_FEATURE_LEVEL_11_0;

		D3D_FEATURE_LEVEL outFeatureLevel;

		CheckDXResult(D3D11CreateDevice(
			0,
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			flags,
			features,
			ARRAYSIZE(features),
			D3D11_SDK_VERSION,
			&_device,
			&outFeatureLevel,
			&_context
		));

		if (outFeatureLevel != D3D_FEATURE_LEVEL_11_0) return false;
		return true;
	}

	bool DirectXDevice::Destroy()
	{
		COM_RELEASE(_context);
		COM_RELEASE(_device);
		return true;
	}

	bool DirectXDevice::CreateSwapchain(DXGI_SWAP_CHAIN_DESC & desc, IDXGISwapChain ** ppHandle)
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

	bool DirectXDevice::CreateRenderTargetView(D3D11_RENDER_TARGET_VIEW_DESC & desc, ID3D11Texture2D * pResource, ID3D11RenderTargetView ** ppHandle)
	{
		CheckDXResult(_device->CreateRenderTargetView(pResource, &desc, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateDepthStencilView(D3D11_DEPTH_STENCIL_VIEW_DESC & desc, ID3D11Texture2D * pResource, ID3D11DepthStencilView ** ppHandle)
	{
		CheckDXResult(_device->CreateDepthStencilView(pResource, &desc, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateBuffer(D3D11_BUFFER_DESC & desc, D3D11_SUBRESOURCE_DATA* pSubData, ID3D11Buffer ** ppHandle)
	{
		CheckDXResult(_device->CreateBuffer(&desc, pSubData, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateSampler(D3D11_SAMPLER_DESC & desc, ID3D11SamplerState ** ppHandle)
	{
		CheckDXResult(_device->CreateSamplerState(&desc, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateVertexShader(void * pCompiledCode, UINT codeSize, ID3D11VertexShader ** ppHandle)
	{
		CheckDXResult(_device->CreateVertexShader(pCompiledCode, codeSize, 0, ppHandle));
		return true;
	}

	bool DirectXDevice::CreatePixelShader(void * pCompiledCode, UINT codeSize, ID3D11PixelShader ** ppHandle)
	{
		CheckDXResult(_device->CreatePixelShader(pCompiledCode, codeSize, 0, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateGeometryShader(void * pCompiledCode, UINT codeSize, ID3D11GeometryShader ** ppHandle)
	{
		CheckDXResult(_device->CreateGeometryShader(pCompiledCode, codeSize, 0, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* pElements, UINT numElements, void * pCompiledVertexShaderCode, UINT codeSize, ID3D11InputLayout ** ppHandle)
	{
		CheckDXResult(_device->CreateInputLayout(pElements, numElements, pCompiledVertexShaderCode, codeSize, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateTexture2D(D3D11_TEXTURE2D_DESC & desc, D3D11_SUBRESOURCE_DATA* subData, ID3D11Texture2D ** ppHandle)
	{
		CheckDXResult(_device->CreateTexture2D(&desc, subData, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateShaderResourceView(ID3D11Resource* pResource, D3D11_SHADER_RESOURCE_VIEW_DESC & desc, ID3D11ShaderResourceView ** ppHandle)
	{
		CheckDXResult(_device->CreateShaderResourceView(pResource, &desc, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateRasterizerState(D3D11_RASTERIZER_DESC & desc, ID3D11RasterizerState ** ppHandle)
	{
		CheckDXResult(_device->CreateRasterizerState(&desc, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& desc, ID3D11DepthStencilState** ppHandle)
	{
		CheckDXResult(_device->CreateDepthStencilState(&desc, ppHandle));
		return true;
	}

	bool DirectXDevice::CreateBlendState(D3D11_BLEND_DESC& desc, ID3D11BlendState** ppHandle)
	{
		CheckDXResult(_device->CreateBlendState(&desc, ppHandle));
		return true;
	}

	bool DirectXDevice::Map(ID3D11Resource* pResource, UINT subresource, D3D11_MAP mapType, UINT mapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
	{
		CheckDXResult(_context->Map(pResource, subresource, mapType, mapFlags, pMappedResource));
		return true;
	}

	bool DirectXDevice::Unmap(ID3D11Resource* pResource, UINT subresource)
	{
		_context->Unmap(pResource, subresource);
		return true;
	}

	bool DirectXDevice::AllocateCommandBuffer(DirectXCommandBuffer * cmdBuffer)
	{
		cmdBuffer->_context = _context;
		return false;
	}

	bool DirectXDevice::VSSetConstantBuffers(UINT startSlot, UINT count, ID3D11Buffer** pBuffers)
	{
		_context->VSSetConstantBuffers(startSlot, count, pBuffers);
		return true;
	}

	bool DirectXDevice::PSSetConstantBuffers(UINT startSlot, UINT count, ID3D11Buffer** pBuffers)
	{
		_context->PSSetConstantBuffers(startSlot, count, pBuffers);
		return true;
	}

	bool DirectXDevice::GSSetConstantBuffers(UINT startSlot, UINT count, ID3D11Buffer ** pBuffers)
	{
		_context->GSSetConstantBuffers(startSlot, count, pBuffers);
		return true;
	}

	bool DirectXDevice::VSSetSamplers(UINT startSlot, UINT count, ID3D11SamplerState** pSamplers)
	{
		_context->VSSetSamplers(startSlot, count, pSamplers);
		return true;
	}

	bool DirectXDevice::PSSetSamplers(UINT startSlot, UINT count, ID3D11SamplerState** pSamplers)
	{
		_context->PSSetSamplers(startSlot, count, pSamplers);
		return true;
	}

	bool DirectXDevice::GSSetSamplers(UINT startSlot, UINT count, ID3D11SamplerState ** pSamplers)
	{
		_context->GSSetSamplers(startSlot, count, pSamplers);
		return true;
	}

	bool DirectXDevice::VSSetShaderResources(UINT startSlot, UINT count, ID3D11ShaderResourceView** pResources)
	{
		_context->VSSetShaderResources(startSlot, count, pResources);
		return true;
	}

	bool DirectXDevice::PSSetShaderResources(UINT startSlot, UINT count, ID3D11ShaderResourceView** pResources)
	{
		_context->PSSetShaderResources(startSlot, count, pResources);
		return true;
	}

	bool DirectXDevice::GSSetShaderResources(UINT startSlot, UINT count, ID3D11ShaderResourceView ** pResources)
	{
		_context->GSSetShaderResources(startSlot, count, pResources);
		return true;
	}

	bool DirectXDevice::UpdateSubresource(ID3D11Resource* pResource, UINT dstSubresource, D3D11_BOX* pDstBox, const void* pSrcData, UINT srcPitchRow, UINT srcDepthPitch)
	{
		_context->UpdateSubresource(pResource, dstSubresource, pDstBox, pSrcData, srcPitchRow, srcDepthPitch);
		return true;
	}

	bool DirectXDevice::GenerateMips(ID3D11ShaderResourceView *pResource)
	{
		_context->GenerateMips(pResource);
		return true;
	}

	bool DirectXDevice::CopyResource(ID3D11Resource* pDst, ID3D11Resource* pSrc)
	{
		_context->CopyResource(pDst, pSrc);
		return true;
	}

	const String &DirectXDevice::GetErrorMsg() const
	{
		return _errMsg;
	}

	String DirectXDevice::QueryAPIError()
	{
		String errStr;

#ifdef _DEBUG
		ID3D11InfoQueue* pInfoQueue = 0;
		_device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue);
		UINT64 msgCount = pInfoQueue->GetNumStoredMessages();
		for (UINT64 msg = 0; msg < msgCount; msg++)
		{
			// Get the size of the message
			SIZE_T messageLength = 0;
			HRESULT hr = pInfoQueue->GetMessage(msg, NULL, &messageLength);

			// Allocate space and get the message
			D3D11_MESSAGE * pMessage = (D3D11_MESSAGE*)malloc(messageLength);
			hr = pInfoQueue->GetMessage(msg, pMessage, &messageLength);

			if (
				pMessage->Severity == D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_ERROR ||
				pMessage->Severity == D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_CORRUPTION ||
				pMessage->Severity == D3D11_MESSAGE_SEVERITY::D3D11_MESSAGE_SEVERITY_WARNING)
			{
				errStr += pMessage->pDescription;

				if(msg + 1 != msgCount)
					errStr += "\n";
			}

			free(pMessage);
		}

		if (errStr.length())
			pInfoQueue->ClearStoredMessages();
		COM_RELEASE(pInfoQueue);
#endif

		return errStr;
	}
}