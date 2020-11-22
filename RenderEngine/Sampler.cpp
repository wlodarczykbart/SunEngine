#include "ISampler.h"
#include "Sampler.h"

namespace SunEngine
{
	Sampler::Sampler() : GraphicsObject(GraphicsObject::SAMPLER)
	{
		_iSampler = 0;;
	}


	Sampler::~Sampler()
	{
		Destroy();
	}

	bool Sampler::Create(const CreateInfo & info)
	{
		if (!Destroy())
			return false;

		ISamplerCreateInfo apiInfo = {};
		apiInfo.settings = info.settings;

		_iSampler = AllocateGraphics<ISampler>();

		if (!_iSampler->Create(apiInfo)) return false;

		_settings = info.settings;
		return true;
	}

	bool Sampler::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_iSampler = 0;
		return true;
	}


	IObject * Sampler::GetAPIHandle() const
	{
		return _iSampler;
	}

	FilterMode Sampler::GetFilter() const
	{
		return _settings.filterMode;
	}

	WrapMode Sampler::GetWrap() const
	{
		return _settings.wrapMode;
	}

	AnisotropicMode Sampler::GetAnisotropy() const
	{
		return _settings.anisotropicMode;
	}

}