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

	enum ShaderPropType
	{
		SPT_TEXTURE2D,
		SPT_SAMPLER,
		SPT_FLOAT,
		SPT_FLOAT2,
		SPT_FLOAT3,
		SPT_FLOAT4,
	};

	struct ShaderProp
	{
		ShaderPropType Type;
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

	class Shader
	{
	public:
		static const String Default;
		static const String GBuffer;
		static const String Depth;

		Shader();
		~Shader();

		bool Compile(const String& vertexSource, const String& pixelSource, String* pErrStr = 0, Vector<String>* pDefines = 0);
		bool Compile(const String& path, String* pErrStr = 0, Vector<String>* pDefines = 0);
		bool Compile(const ConfigFile& config, String* pErrStr = 0, Vector<String>* pDefines = 0);

		void SetDefaults(Material* pMtl) const;

		BaseShader* GetDefault() const;
		BaseShader* GetVariant(const String& variant) const;

		bool GetConfigSection(const String& name, ConfigSection& section) const;
		bool GetVariantProps(const String& name, StrMap<ShaderProp>& props) const;
		bool GetVariantDefines(const String& name, Vector<String>& defines) const;

		const String& GetName() const { return _name; }

	private:
		void CollectConfigFiles(LinkedList<ConfigFile>& configList, HashSet<String>& configMap);
		void SetDefaults();
		void ParseSamplerAnisotropy(const String& str, FilterMode& fm, WrapMode& wm, AnisotropicMode& am) const;
		void ParseFloats(const String& str, uint maxComponents, float* pData) const;

		struct ShaderVariant
		{
			BaseShader shader;
			StrMap<ShaderProp> defaults;
			Vector<String> defines;
		};

		String _name;
		ConfigFile _config;
		StrMap<UniquePtr<ShaderVariant>> _variants;
	};
}