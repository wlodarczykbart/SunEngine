#pragma once

#include "BaseShader.h"
#include "GPUResource.h"
#include "UniformBuffer.h"
#include "Shader.h"

namespace SunEngine
{
	class Texture2D;
	class TextureCube;
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
		static const String PositionMap;
		static const String DepthMap;
		static const String AlphaMap;
	};

	class Material : public GPUResource<ShaderBindings>
	{
	public:
		struct MaterialTextureData
		{
			union
			{
				Texture2D* pTexture;
				TextureCube* pTextureCube;
			};
			IShaderResource Res;
		};

		Material();
		~Material();

		//Set shader and let the engine determine what variant to use depending on rendering mode, which should only var between application runs
		void SetShader(Shader* pShader);
		//Set shader with specified variant
		void SetShader(Shader* pShader, const String& variant);
		Shader* GetShader() const { return _shader; }
		const String& GetVariant() const { return _variant; }
		BaseShader* GetShaderVariant() const;

		template<typename T>
		bool SetMaterialVar(const String& name, const T value)
		{
			return SetMaterialVar(name, &value, sizeof(T));
		}

		template<typename T>
		bool GetMaterialVar(const String& name, T& value)
		{
			return GetMaterialVar(name, &value, sizeof(T));
		}	

		Texture2D* GetTexture2D(const String& name) const { auto found = _mtlTextures2D.find(name); return found != _mtlTextures2D.end() ? (*found).second.pTexture : 0; }

		bool SetMaterialVar(const String& name, const void* pData, uint size);
		bool GetMaterialVar(const String& name, void* pData, uint size) const;

		bool SetTexture2D(const String& name, Texture2D* pTexture);
		bool SetTextureCube(const String& name, TextureCube* pTexture);
		bool SetSampler(const String& name, Sampler* pSampler);

		bool RegisterToGPU() override;

		StrMap<ShaderBufferVariable>::const_iterator BeginVars() const { return _mtlVariables.begin(); };
		StrMap<ShaderBufferVariable>::const_iterator EndVars() const { return _mtlVariables.end(); };

		StrMap<MaterialTextureData>::const_iterator BeginTextures2D() const { return _mtlTextures2D.begin(); };
		StrMap<MaterialTextureData>::const_iterator EndTextures2D() const { return _mtlTextures2D.end(); };

		bool Write(StreamBase& stream) override;
		bool Read(StreamBase& stream) override;

		usize GetDepthVariantHash() const { return _depthVariantHash; }
		bool CreateDepthMaterial(Material* pEmptyMaterial) const;
		
	private:
		void UpdateDepthVariant(const String& name, const ShaderProp& prop);
		void UpdateDepthVariantHash();

		struct MaterialSamplerData
		{
			IShaderResource Res;
		};

		Shader* _shader;
		String _variant;
		MemBuffer _memBuffer;
		StrMap<ShaderBufferVariable> _mtlVariables;
		StrMap<MaterialTextureData> _mtlTextures2D;
		StrMap<MaterialTextureData> _mtlTextureCubes;
		StrMap<MaterialSamplerData> _mtlSamplers;

		StrMap<ShaderProp> _depthVariantProps;
		usize _depthVariantDefineHash;
		usize _depthVariantHash;

		UniformBuffer _mtlBuffer;
	};
}