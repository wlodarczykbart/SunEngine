#include "D3D11Device.h"

#include "D3D11Object.h"
#include "D3D11CommandBuffer.h"
#include "StringUtil.h"

#define CheckDXResult(expression) if(expression != S_OK) { _errMsg = StrFormat("Error on line %d, %s\n",  __LINE__, #expression); return false; }

namespace SunEngine
{
	D3D11Device::D3D11Device()
	{
		_device = 0;
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
			ARRAYSIZE(features),
			D3D11_SDK_VERSION,
			&device,
			&outFeatureLevel,
			&context
		));

		if (outFeatureLevel != D3D_FEATURE_LEVEL_11_1) return false;

		if (device->QueryInterface(__uuidof(ID3D11Device1), (void**)&_device) != S_OK) return false;
		if (context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&_context) != S_OK) return false;

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

	bool D3D11Device::VSSetConstantBuffer(UINT startSlot, ID3D11Buffer* pBuffer, uint firstConstant, uint numConstants)
	{
		if (firstConstant == 0)
			_context->VSSetConstantBuffers(startSlot, 1, &pBuffer);
		else
			_context->VSSetConstantBuffers1(startSlot, 1, &pBuffer, &firstConstant, &numConstants);
		return true;
	}

	bool D3D11Device::PSSetConstantBuffer(UINT startSlot, ID3D11Buffer* pBuffer, uint firstConstant, uint numConstants)
	{
		if (firstConstant == 0)
			_context->PSSetConstantBuffers(startSlot, 1, &pBuffer);
		else
			_context->PSSetConstantBuffers1(startSlot, 1, &pBuffer, &firstConstant, &numConstants);
		return true;
	}

	bool D3D11Device::GSSetConstantBuffer(UINT startSlot, ID3D11Buffer * pBuffer, uint firstConstant, uint numConstants)
	{
		if (firstConstant == 0)
			_context->GSSetConstantBuffers(startSlot, 1, &pBuffer);
		else
			_context->GSSetConstantBuffers1(startSlot, 1, &pBuffer, &firstConstant, &numConstants);
		return true;
	}

	bool D3D11Device::VSSetSampler(UINT startSlot, ID3D11SamplerState* pSampler)
	{
		_context->VSSetSamplers(startSlot, 1, &pSampler);
		return true;
	}

	bool D3D11Device::PSSetSampler(UINT startSlot, ID3D11SamplerState* pSampler)
	{
		_context->PSSetSamplers(startSlot, 1, &pSampler);
		return true;
	}

	bool D3D11Device::GSSetSampler(UINT startSlot, ID3D11SamplerState * pSampler)
	{
		_context->GSSetSamplers(startSlot, 1, &pSampler);
		return true;
	}

	bool D3D11Device::VSSetShaderResource(UINT startSlot, ID3D11ShaderResourceView* pResource)
	{
		_context->VSSetShaderResources(startSlot, 1, &pResource);
		return true;
	}

	bool D3D11Device::PSSetShaderResource(UINT startSlot, ID3D11ShaderResourceView* pResource)
	{
		_context->PSSetShaderResources(startSlot, 1, &pResource);
		return true;
	}

	bool D3D11Device::GSSetShaderResource(UINT startSlot, ID3D11ShaderResourceView * pResource)
	{
		_context->GSSetShaderResources(startSlot, 1, &pResource);
		return true;
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