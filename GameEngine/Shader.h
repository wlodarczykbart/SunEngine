#pragma once

#include "glm/glm.hpp"
#include "BaseShader.h"
#include "ConfigFile.h"
#include "GPUResource.h"

namespace SunEngine
{
	class Material;
	class Texture2D;
	class Sampler;

	class Shader : public GPUResource<BaseShader>
	{
	public:
		Shader();
		~Shader();

		bool LoadConfig(const String& path);
		void SetCreateInfo(const BaseShader::CreateInfo& info);

		bool RegisterToGPU() override;

		void SetDefaults(Material* pMtl) const;

		const ConfigSection* GetConfigSection(const String& name) const { return _config.GetSection(name.c_str()); }

	private:
		void SetDefaults();
		void ParseSamplerAnisotropy(const String& str, FilterMode& fm, WrapMode& wm, AnisotropicMode& am) const;
		void ParseFloats(const String& str, uint maxComponents, float* pData) const;

		enum DefaultValueDataType
		{
			DV_TEXTURE2D,
			DV_SAMPLER,
			DV_FLOAT,
			DV_FLOAT2,
			DV_FLOAT3,
			DV_FLOAT4,
		};

		struct DefaultValue
		{
			DefaultValueDataType Type;
			union
			{
				Texture2D* pTexture2D;
				Sampler* pSampler;
				float float1;
				glm::vec2 float2;
				glm::vec3 float3;
				glm::vec4 float4;
			};
		};

		BaseShader::CreateInfo _createInfo;
		ConfigFile _config;
		StrMap<DefaultValue> _defaultValues;
	};
}