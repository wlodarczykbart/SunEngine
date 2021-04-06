#pragma once

#include "BaseShader.h"
#include "GPUResource.h"
#include "UniformBuffer.h"
#include "Shader.h"

namespace SunEngine
{
	class Texture2D;
	class Sampler;

	class MaterialStrings
	{
	public:
		static const String DiffuseMap;
		static const String NormalMap;
		static const String SpecularMap;
		static const String MetalMap;
		static const String AmbientOcclusionMap;
		static const String SmoothnessMap;
		static const String DiffuseColor;
		static const String SpecularColor;
		static const String Smoothness;
		static const String Sampler;
	};

	class Material : public GPUResource<ShaderBindings>
	{
	public:
		struct MaterialTextureData
		{
			Texture2D* pTexture;
			IShaderResource Res;
		};

		Material();
		~Material();

		void SetShader(Shader* pShader, const String& variant = Shader::Default);
		Shader* GetShader() const { return _shader; }
		const String& GetVariant() const { return _variant; }

		template<typename T>
		bool SetMaterialVar(const String& name, const T value)
		{
			return SetMaterialVar(name, &value, sizeof(T));
		}

		template<typename T>
		bool GetMaterialVar(const String& name, T& value)
		{
			auto found = _mtlVariables.find(name);
			if (found != _mtlVariables.end())
			{
				memcpy(&value, _memBuffer.GetData((*found).second.offset), (*found).second.size);
				return true;
			}
			else
			{
				return false;
			}
		}	

		Texture2D* GetTexture2D(const String& name) const { auto found = _mtlTextures2D.find(name); return found != _mtlTextures2D.end() ? (*found).second.pTexture : 0; }

		bool SetMaterialVar(const String& name, const void* pData, const uint size);
		bool SetTexture2D(const String& name, Texture2D* pTexture);
		bool SetSampler(const String& name, Sampler* pSampler);

		bool RegisterToGPU() override;

		void BuildPipelineSettings(PipelineSettings& settings) const;

		StrMap<ShaderBufferVariable>::const_iterator BeginVars() const { return _mtlVariables.begin(); };
		StrMap<ShaderBufferVariable>::const_iterator EndVars() const { return _mtlVariables.end(); };

		StrMap<MaterialTextureData>::const_iterator BeginTextures2D() const { return _mtlTextures2D.begin(); };
		StrMap<MaterialTextureData>::const_iterator EndTextures2D() const { return _mtlTextures2D.end(); };

		bool Write(StreamBase& stream) override;
		bool Read(StreamBase& stream) override;
		
	private:

		struct MaterialSamplerData
		{
			IShaderResource Res;
		};

		Shader* _shader;
		String _variant;
		MemBuffer _memBuffer;
		StrMap<ShaderBufferVariable> _mtlVariables;
		StrMap<MaterialTextureData> _mtlTextures2D;
		StrMap<MaterialSamplerData> _mtlSamplers;

		UniformBuffer _mtlBuffer;
	};
}