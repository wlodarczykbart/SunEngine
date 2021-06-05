#pragma once

#include "GraphicsAPIDef.h"

namespace SunEngine
{
	class BaseTexture;
	class Sampler;

	class GraphicsContext
	{
	public:
		struct CreateInfo
		{
			bool debugEnabled;
			bool vrEnabled;
		};

		enum DefaultTexture
		{
			DT_WHITE,
			DT_BLACK,

			DT_COUNT
		};

		enum DefaultSampler
		{
			DS_NEAR_CLAMP,
			DS_NEAR_REPEAT,
			DS_LINEAR_CLAMP,
			DS_LINEAR_REPEAT,

			DS_COUNT
		};

		GraphicsContext();
		~GraphicsContext();

		bool Create(const CreateInfo& createInfo);
		bool CreateDefaultObjects();
		bool Destroy();

		const String& GetErrStr() const;
		String QueryAPIError() const;

		static IDevice* GetDevice();
		static BaseTexture* GetDefaultTexture(DefaultTexture texture);
		static Sampler* GetDefaultSampler(DefaultSampler sampler);
		static const char* GetAPIName();

	private:
		static GraphicsContext* _singleton;

		IDevice* _iDevice;

		BaseTexture* _defaultTextures[DT_COUNT];
		Sampler* _defaultSamplers[DS_COUNT];
	};

}