#include "D3D11Shader.h"
#include "D3D11Sampler.h"

#define MIN_MAG_MIP(_minFilter,_magFilter,_mipmapMode, d3dFilter) if((settings.minFilter == _minFilter && settings.magFilter == _magFilter && settings.mipmapMode == _mipmapMode)) { desc.Filter = d3dFilter; }

namespace SunEngine
{
	Map<WrapMode, D3D11_TEXTURE_ADDRESS_MODE> AddressMap
	{
		{ SE_WM_CLAMP_TO_EDGE, D3D11_TEXTURE_ADDRESS_CLAMP },
		{ SE_WM_REPEAT, D3D11_TEXTURE_ADDRESS_WRAP }
	};

	D3D11Sampler::D3D11Sampler()
	{
	}

	D3D11Sampler::~D3D11Sampler()
	{
		Destroy();
	}

	bool D3D11Sampler::Create(const ISamplerCreateInfo & info)
	{
		SamplerSettings settings = info.settings;

		D3D11_SAMPLER_DESC desc = {};
		desc.AddressU = AddressMap[settings.wrapMode];
		desc.AddressV = AddressMap[settings.wrapMode];
		desc.AddressW = AddressMap[settings.wrapMode];
		desc.MaxAnisotropy = 1;
		desc.MinLOD = 0*-FLT_MAX;
		desc.MaxLOD = +FLT_MAX;
		
		if (settings.filterMode == SE_FM_LINEAR)
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		else if (settings.filterMode == SE_FM_NEAREST)
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

		if (settings.anisotropicMode != SE_AM_OFF)
		{
			desc.Filter = D3D11_FILTER_ANISOTROPIC;
			desc.MaxAnisotropy = (1 << settings.anisotropicMode);
		}

		if (!_device->CreateSampler(desc, &_sampler)) return false;

		return true;
	}

	bool D3D11Sampler::Destroy()
	{
		COM_RELEASE(_sampler);
		return true;
	}

	void D3D11Sampler::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void D3D11Sampler::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	void D3D11Sampler::BindToShader(D3D11Shader* pShader, const String&, uint binding, IBindState*)
	{
		pShader->BindSampler(this, binding);
	}

}