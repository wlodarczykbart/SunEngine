#pragma once

#include "SamplerSettings.h"
#include "GraphicsObject.h"

namespace SunEngine
{
	class Sampler : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			SamplerSettings settings;
		};

		Sampler();
		~Sampler();

		bool Create(const CreateInfo &info);
		bool Destroy() override;

		IObject* GetAPIHandle() const override;

		FilterMode GetFilter() const;
		WrapMode GetWrap() const;
		AnisotropicMode GetAnisotropy() const;

	private:
		ISampler* _iSampler;
		SamplerSettings _settings;
	};

}