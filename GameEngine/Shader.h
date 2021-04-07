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

	class Shader
	{
	public:
		static const String Default;
		static const String Deferred;
		static const String Shadow;

		Shader();
		~Shader();

		bool Compile(const String& vertexSource, const String& pixelSource, String* pErrStr = 0);
		bool Compile(const String& path, String* pErrStr = 0);
		bool Compile(const ConfigFile& config, String* pErrStr = 0);

		void SetDefaults(Material* pMtl) const;

		BaseShader* GetDefault() const;
		BaseShader* GetVariant(const String& variant) const;

		bool GetConfigSection(const String& name, ConfigSection& section) const;

	private:
		void CollectConfigFiles(LinkedList<ConfigFile>& configList, HashSet<String>& configMap);
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

		struct ShaderVariant
		{
			BaseShader shader;
			StrMap<DefaultValue> defaults;
		};

		ConfigFile _config;
		StrMap<UniquePtr<ShaderVariant>> _variants;
	};
}