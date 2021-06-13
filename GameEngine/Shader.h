#pragma once

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

	namespace ShaderVariant
	{
		enum
		{
			GBUFFER = 1 << 0,
			DEPTH = 1 << 1,
			ALPHA_TEST = 1 << 2,
			SKINNED = 1 << 3,
			ONE_Z = 1 << 4,
		};
	}

	class Shader
	{
	public:

		Shader();
		~Shader();

		bool Compile(const String& vertexSource, const String& pixelSource, String* pErrStr = 0, Vector<String>* pDefines = 0);
		bool Compile(const String& path, String* pErrStr = 0, Vector<String>* pDefines = 0);
		bool Compile(const ConfigFile& config, String* pErrStr = 0, Vector<String>* pDefines = 0);

		void SetDefaults(Material* pMtl) const;

		BaseShader* GetBase() const;
		BaseShader* GetBaseVariant(uint64 variantMask) const;

		bool GetConfigSection(const String& name, ConfigSection& section) const;
		bool GetVariantProps(uint64 variantMask, StrMap<ShaderProp>** props) const;
		//bool GetVariantDefines(uint64 variantMask, Vector<String>& defines) const;
		bool ContainsVariants(uint64 variantMask) const;

		const String& GetName() const { return _name; }

		static void FillMatrices(const glm::mat4& view, const glm::mat4& proj, CameraBufferData& camData);

	private:
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
		Map<uint64, UniquePtr<ShaderVariant>> _variants;
	};
}