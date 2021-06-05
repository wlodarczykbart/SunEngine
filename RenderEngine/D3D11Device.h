#pragma once

#include "GraphicsAPIDef.h"

#include "Types.h"
#include "GraphicsWindow.h"
#include <d3d11_1.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "IDevice.h"

#define COM_RELEASE(dx) if(dx) {dx->Release(); dx = 0;}
#define CheckDXResult(expression) if(expression != S_OK) { _errMsg = StrFormat("Error on line %d, %s\n",  __LINE__, #expression); return false; }

namespace SunEngine
{
	class D3D11Object;
	class D3D11CommandBuffer;

	class D3D11Device : public IDevice
	{
	public:
		D3D11Device();
		~D3D11Device();

		bool Create(const IDeviceCreateInfo& info) override;
		bool Destroy() override;

		const String &GetErrorMsg() const override;
		String QueryAPIError() override;
		bool WaitIdle() override;

		bool CreateSwapchain(DXGI_SWAP_CHAIN_DESC &desc, IDXGISwapChain** ppHandle);
		bool CreateRenderTargetView(D3D11_RENDER_TARGET_VIEW_DESC& desc, ID3D11Texture2D* pResource, ID3D11RenderTargetView** ppHandle);
		bool CreateDepthStencilView(D3D11_DEPTH_STENCIL_VIEW_DESC& desc, ID3D11Texture2D* pResource, ID3D11DepthStencilView** ppHandle);
		bool CreateBuffer(D3D11_BUFFER_DESC& desc, D3D11_SUBRESOURCE_DATA* pSubData, ID3D11Buffer** ppHandle);
		bool CreateSampler(D3D11_SAMPLER_DESC& desc, ID3D11SamplerState** ppHandle);
		bool CreateVertexShader(void* pCompiledCode, UINT codeSize, ID3D11VertexShader **ppHandle);
		bool CreatePixelShader(void* pCompiledCode, UINT codeSize, ID3D11PixelShader **ppHandle);
		bool CreateGeometryShader(void * pCompiledCode, UINT codeSize, ID3D11GeometryShader ** ppHandle);
		bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* pElements, UINT numElements, void* pCompiledVertexShaderCode, UINT codeSize, ID3D11InputLayout **ppHandle);
		bool CreateTexture2D(D3D11_TEXTURE2D_DESC& desc, D3D11_SUBRESOURCE_DATA* subData, ID3D11Texture2D** ppHandle);
		bool CreateShaderResourceView(ID3D11Resource* pResource, D3D11_SHADER_RESOURCE_VIEW_DESC& desc, ID3D11ShaderResourceView** ppHandle);
		bool CreateRasterizerState(D3D11_RASTERIZER_DESC& desc, ID3D11RasterizerState** ppHandle);
		bool CreateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& desc, ID3D11DepthStencilState** ppHandle);
		bool CreateBlendState(D3D11_BLEND_DESC& desc, ID3D11BlendState** ppHandle);


		bool Map(ID3D11Resource* pResource, UINT subresource, D3D11_MAP mapType, UINT mapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource);
		bool Unmap(ID3D11Resource* pResource, UINT subresource);
		bool AllocateCommandBuffer(D3D11CommandBuffer* cmdBuffer);

		bool VSSetConstantBuffer(UINT startSlot, ID3D11Buffer* pBuffer, uint firstConstant, uint numConstants);
		bool PSSetConstantBuffer(UINT startSlot, ID3D11Buffer* pBuffer, uint firstConstant, uint numConstants);
		bool GSSetConstantBuffer(UINT startSlot, ID3D11Buffer* pBuffer, uint firstConstant, uint numConstants);
		bool VSSetSampler(UINT startSlot, ID3D11SamplerState* pSampler);
		bool PSSetSampler(UINT startSlot, ID3D11SamplerState* pSampler);
		bool GSSetSampler(UINT startSlot, ID3D11SamplerState* pSampler);
		bool VSSetShaderResource(UINT startSlot, ID3D11ShaderResourceView* pResource);
		bool PSSetShaderResource(UINT startSlot, ID3D11ShaderResourceView* pResource);
		bool GSSetShaderResource(UINT startSlot, ID3D11ShaderResourceView* pResource);

		bool UpdateSubresource(ID3D11Resource* pResource, UINT dstSubresource, D3D11_BOX* pDstBox, const void* pSrcData, UINT srcPitchRow, UINT srcDepthPitch);
		bool GenerateMips(ID3D11ShaderResourceView *pResource);
		bool CopyResource(ID3D11Resource* pDst, ID3D11Resource* pSrc);

		bool FillSampleDesc(DXGI_FORMAT format, uint samples, DXGI_SAMPLE_DESC& desc);

	private:
		friend class D3D11VRInterface;

		ID3D11Device1* _device;
		ID3D11DeviceContext1* _context;

		String _errMsg;
		String _errLine;
	};

}