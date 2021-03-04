#include "IDevice.h"
#include "BaseTexture.h"
#include "Sampler.h"
#include "GraphicsContext.h"

namespace SunEngine
{
	GraphicsContext* GraphicsContext::_singleton = 0;

	GraphicsContext::GraphicsContext()
	{
		_iDevice = 0;
		memset(_defaultTextures, 0, sizeof(_defaultTextures));
		memset(_defaultSamplers, 0, sizeof(_defaultSamplers));
	}


	GraphicsContext::~GraphicsContext()
	{
	}

	bool GraphicsContext::Create(const CreateInfo& createInfo)
	{
		_singleton = this;

		if (!_iDevice)
		{
			_iDevice = AllocateGraphics<IDevice>();
		}
		else
		{
			_iDevice->Destroy();
		}

		IDeviceCreateInfo deviceInfo = {};
		deviceInfo.debugEnabled = createInfo.debugEnabled;

		if (!_iDevice->Create(deviceInfo))
		{
			return false;
		}

		Vector<Pixel> defaultPixels[DT_COUNT];
		
		defaultPixels[DT_BLACK] =
		{
			{ 0, 0, 0, 255, }, { 0, 0, 0, 255, },
			{ 0, 0, 0, 255, }, { 0, 0, 0, 255, }
		};

		defaultPixels[DT_WHITE] =
		{
			{ 255, 255, 255, 255, }, { 255, 255, 255, 255, },
			{ 255, 255, 255, 255, }, { 255, 255, 255, 255, }
		};

		for (uint i = 0; i < DT_COUNT; i++)
		{
			BaseTexture::CreateInfo texInfo = {};
			texInfo.image.Width = 2;
			texInfo.image.Height = 2;
			texInfo.image.Pixels = defaultPixels[i].data();

			BaseTexture* pTexture = new BaseTexture();
			if (!pTexture->Create(texInfo))
				return false;

			_defaultTextures[i] = pTexture;
		}

		Pair<FilterMode, WrapMode> defaultSamplerModes[DS_COUNT];
		defaultSamplerModes[DS_NEAR_CLAMP] = { SE_FM_NEAREST, SE_WM_CLAMP_TO_EDGE };
		defaultSamplerModes[DS_NEAR_REPEAT] = { SE_FM_NEAREST, SE_WM_REPEAT };
		defaultSamplerModes[DS_LINEAR_CLAMP] = { SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE };
		defaultSamplerModes[DS_LINEAR_REPEAT] = { SE_FM_LINEAR, SE_WM_REPEAT };

		for (uint i = 0; i < DS_COUNT; i++)
		{
			Sampler::CreateInfo samplerInfo = {};
			samplerInfo.settings.filterMode = defaultSamplerModes[i].first;
			samplerInfo.settings.wrapMode = defaultSamplerModes[i].second;
			samplerInfo.settings.anisotropicMode = SE_AM_OFF;

			Sampler* pSampler = new Sampler();
			if (!pSampler->Create(samplerInfo))
				return false;

			_defaultSamplers[i] = pSampler;
		}

		return true;
	}

	bool GraphicsContext::Destroy()
	{
		_singleton = 0;

		if (!_iDevice)
			return true;

		for (uint i = 0; i < DT_COUNT; i++)
		{
			if (!_defaultTextures[i]->Destroy())
				return false;
			delete _defaultTextures[i];
		}

		for (uint i = 0; i < DS_COUNT; i++)
		{
			if (!_defaultSamplers[i]->Destroy())
				return false;
			delete _defaultSamplers[i];
		}

		if (!_iDevice->Destroy())
			return false;

		_iDevice = 0;
		return true;
	}

	const String &GraphicsContext::GetErrStr() const
	{
		return _iDevice->GetErrorMsg();
	}

	String GraphicsContext::QueryAPIError() const
	{
		return _iDevice->QueryAPIError();
	}

	IDevice * GraphicsContext::GetDevice()
	{
		if (!_singleton)
			return 0;

		return _singleton->_iDevice;
	}

	BaseTexture* GraphicsContext::GetDefaultTexture(DefaultTexture texture)
	{
		if (!_singleton)
			return 0;

		return _singleton->_defaultTextures[texture];
	}

	Sampler * GraphicsContext::GetDefaultSampler(DefaultSampler sampler)
	{
		if (!_singleton)
			return 0;

		return _singleton->_defaultSamplers[sampler];
	}

	const char* GraphicsContext::GetAPIName()
	{
		switch (GetGraphicsAPI())
		{
		case SE_GFX_D3D11:
			return "D3D11";
		case SE_GFX_VULKAN:
			return "Vulkan";
		default:
			return "";
		}
	}

}
