#include "D3D11Device.h"
#define XR_USE_GRAPHICS_API_D3D11
#include "openxr/openxr.h"
#include "openxr/openxr_platform.h"

#include "D3D11Texture.h"
#include "D3D11VRInterface.h"

namespace SunEngine
{
	const char* D3D11VRInterface::GetExtensionName() const
	{
		return XR_KHR_D3D11_ENABLE_EXTENSION_NAME;
	}

	bool D3D11VRInterface::Init(IVRInitInfo& info)
	{
		if (_device->_device || _device->_context)
			return false;

		XrInstance instance = (XrInstance)info.inInstance;
		XrSystemId systemId = (XrSystemId)info.inSystemID;
		PFN_xrGetInstanceProcAddr GetInstanceProcAddr = (PFN_xrGetInstanceProcAddr)info.inGetInstanceProcAddr;

		XrGraphicsRequirementsD3D11KHR requirements = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
		PFN_xrGetD3D11GraphicsRequirementsKHR GetRequirementFunc = 0;
		XR_RETURN_ON_FAIL(GetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&GetRequirementFunc));
		XR_RETURN_ON_FAIL(GetRequirementFunc(instance, systemId, &requirements));

		HRESULT hRes;

		// Turn the LUID into a specific graphics device adapter
		IDXGIAdapter1* adapter = 0;
		IDXGIFactory1* dxgiFactory;
		DXGI_ADAPTER_DESC1 adapterDesc;

		hRes = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgiFactory));
		uint curr = 0;
		while (dxgiFactory->EnumAdapters1(curr++, &adapter) == S_OK)
		{
			hRes = adapter->GetDesc1(&adapterDesc);
			if (memcmp(&adapterDesc.AdapterLuid, &requirements.adapterLuid, sizeof(&requirements.adapterLuid)) == 0)
				break;

			COM_RELEASE(adapter);
		}
		COM_RELEASE(dxgiFactory);

		if (adapter == 0)
			return false;

		UINT flags = 0;

		if (info.inDebugEnabled)
			flags |= D3D11_CREATE_DEVICE_DEBUG;

		D3D_FEATURE_LEVEL features[1];
		features[0] = D3D_FEATURE_LEVEL_11_1;

		D3D_FEATURE_LEVEL outFeatureLevel;

		ID3D11Device* device = 0;
		ID3D11DeviceContext* context = 0;

		hRes = (D3D11CreateDevice(
			adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
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

		hRes = device->QueryInterface(__uuidof(ID3D11Device1), (void**)&_device->_device);
		hRes = context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&_device->_context);

		COM_RELEASE(device);
		COM_RELEASE(context);
		COM_RELEASE(adapter);

		static XrGraphicsBindingD3D11KHR bindings = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
		bindings.device = _device->_device;
		info.outBinding = &bindings;

		info.outSupportedSwapchainFormats =
		{
			DXGI_FORMAT_R8G8B8A8_UNORM,
			DXGI_FORMAT_B8G8R8A8_UNORM,
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		};

		static XrSwapchainImageD3D11KHR imageHeaders[64];
		for (uint i = 0; i < SE_ARR_SIZE(imageHeaders); i++)
			imageHeaders[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
		info.outImageHeaders = imageHeaders;

		return true;
	}

	bool D3D11VRInterface::InitTexture(VRHandle imgArray, uint imgIndex, int64 format, ITexture* pTexture)
	{
		D3D11Texture* pD3DTexture = static_cast<D3D11Texture*>(pTexture);
		XrSwapchainImageD3D11KHR& imgInfo = ((XrSwapchainImageD3D11KHR*)imgArray)[imgIndex];

		pD3DTexture->_texture = imgInfo.texture;

		pD3DTexture->_viewDesc = {};
		pD3DTexture->_viewDesc.Texture2D.MipLevels = 1;
		pD3DTexture->_viewDesc.Format = (DXGI_FORMAT)format;
		pD3DTexture->_viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		if (!_device->CreateShaderResourceView(pD3DTexture->_texture, pD3DTexture->_viewDesc, &pD3DTexture->_srv)) return false;
		pD3DTexture->_external = true;

		return true;
	}

	void D3D11VRInterface::Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState)
	{
	}

	void D3D11VRInterface::Unbind(ICommandBuffer* cmdBuffer)
	{
	}
}