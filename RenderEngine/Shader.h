#pragma once

#include "ConfigFile.h"
#include "GraphicsObject.h"

#define SHADER_VERSION 1

#define SE_DEFAULT_SHADER_PASS "DEFAULT"
#define SE_SHADOW_SHADER_PASS "SHADOW"

#define SE_SCENE_TEXTURE_NAME "SceneTexture"
#define SE_SCENE_SAMPLER_NAME "SceneSampler"
#define SE_DEPTH_TEXTURE_NAME "DepthTexture"
#define SE_DEPTH_SAMPLER_NAME "DepthSampler"
#define SE_SHADOW_TEXTURE_NAME "ShadowTexture"
#define SE_SHADOW_SAMPLER_NAME "ShadowSampler"

namespace SunEngine
{
	struct ShaderMat4
	{
		ShaderMat4();

		float data[16];
	};

	struct ShaderVec4
	{
		ShaderVec4();
		void Set(float x, float y, float z, float w);

		float data[4];
	};

	class UniformBuffer;
	class RenderTarget;
	class Sampler;
	class BaseTexture;
	class BaseTextureCube;
	class BaseTextureArray;

	struct CameraBufferData
	{
		ShaderMat4 ViewMatrix;
		ShaderMat4 ProjectionMatrix;
		ShaderMat4 InvViewMatrix;
		ShaderMat4 InvProjectionMatrix;
	};

	struct ObjectBufferData
	{
		ShaderMat4 WorldMatrix;
		ShaderMat4 InverseTransposeMatrix;
	};


	struct SunlightBufferData
	{
		ShaderVec4 Direction;
		ShaderVec4 Color;
	};

	struct PointLightBufferData
	{
		ShaderVec4 Position;
		ShaderVec4 Color;
		ShaderVec4 Attenuation;
	};

	struct SpotLightBufferData
	{
		ShaderVec4 Position;
		ShaderVec4 DirectionAndAngle;
		ShaderVec4 Attenuation;
		ShaderVec4 Color;
	};

	struct FogBufferData
	{
		ShaderVec4 FogColor;
		ShaderVec4 FogControls;
	};

	struct EnvBufferData
	{
		ShaderVec4 TimeData;
		ShaderVec4 WindDir;
	};

	struct ShadowBufferData
	{
		ShaderMat4 ShadowMatrix;
	};

	struct SampleSceneBufferData
	{
		ShaderVec4 TexCoordTransform;
		ShaderVec4 TexCoordRanges; //x,z = x range    y,w = y range
	};

	static const uint MAX_SHADER_TEXTURE_TRANSFORMS = 32;
	struct TextureTransformBufferData
	{
		ShaderVec4 Transforms[MAX_SHADER_TEXTURE_TRANSFORMS];
	};

	static const uint MAX_SHADER_SKINNED_BONES = 256;
	struct SkinnedBonesBufferData
	{
		ShaderMat4 SkinnedBones[MAX_SHADER_SKINNED_BONES];
	};

	enum ShaderDataType
	{
		SDT_FLOAT,
		SDT_FLOAT2,
		SDT_FLOAT3,
		SDT_FLOAT4,
		SDT_MAT2,
		SDT_MAT3,
		SDT_MAT4,

		SDT_UNDEFINED,
	};
	
	struct ShaderBufferProperty
	{
		String Name;
		ShaderDataType Type;
		uint Size;
		uint Offset;
		uint NumElements;
	};

	class Shader;

	class ShaderBindings : public GraphicsObject
	{
	public:
		ShaderBindings();
		ShaderBindings(const ShaderBindings&) = delete;
		ShaderBindings& operator = (const ShaderBindings&) = delete;
		~ShaderBindings() = default;

		IObject* GetAPIHandle() const override;
		bool OnBind(CommandBuffer* cmdBuffer) override;
		bool OnUnbind(CommandBuffer* cmdBuffer) override;

		bool SetTexture(const String& name, BaseTexture* pTexture);
		bool SetSampler(const String& name, Sampler* pSampler);
		bool SetTextureCube(const String& name, BaseTextureCube* pTextureCube);
		bool SetTextureArray(const String& name, BaseTextureArray* pTextureArray);

		bool Destroy() override;

		inline Shader* GetShader() const { return _shader; }
		//bool UpdateIfChanged();

		bool ContainsResource(const String& name) const;

	private:
		struct ResourceInfo
		{
			ResourceInfo();

			GraphicsObject* Resource;
			IObject* CurrentHandle;
			uint Binding;

			void Set(GraphicsObject* pObject);
		};

		friend class Shader;

		Shader* _shader;
		bool _bDirty;

		StrMap<IShaderBindings*> _iBindings;
		StrMap<ResourceInfo> _resourceMap;
	};

	class Shader : public GraphicsObject
	{
	public:
		Shader();
		~Shader();

		bool Create(const char *configFileName);

		IObject* GetAPIHandle() const override;

		bool GetMaterialBufferDesc(IShaderResource* desc, Vector<ShaderBufferProperty>* pBufferProps = 0) const;
		void GetTextureDescs(Vector<IShaderResource>& desc) const;
		void GetSamplerDescs(Vector<IShaderResource>& desc) const;

		void UpdateCameraBuffer(const CameraBufferData& data);
		void UpdateObjectBuffer(const ObjectBufferData& data);
		void UpdateMaterialBuffer(const void* pData);
		void UpdateSunlightBuffer(const SunlightBufferData& data);
		void UpdateTextureTransformBuffer(const TextureTransformBufferData& data);
		void UpdateFogBuffer(const FogBufferData& data);
		void UpdateSkinnedBoneBuffer(const SkinnedBonesBufferData& data);
		void UpdateEnvBuffer(const EnvBufferData& data);
		void UpdateShadowBuffer(const ShadowBufferData& data);
		void UpdateSampleSceneBuffer(const SampleSceneBufferData& data);

		String GetCompilerCommandLine();
		const String &GetConfigFilename() const;

		bool Destroy() override;

		bool RegisterShaderBindings(ShaderBindings* pBindings);

		bool ContainsTextureBinding(uint binding) const;
		bool ContainsSamplerBinding(uint binding) const;

		const String& GetActiveShaderPass() const;
		void SetActiveShaderPass(const String& passName);

		void SetDefaultShaderPass();
		void SetShadowShaderPass();

		bool SupportsShaderPass(const String& passName);

		bool OnBind(CommandBuffer* cmdBuffer) override;
		bool OnUnbind(CommandBuffer* cmdBuffer) override;

		inline const CameraBufferData& GetCameraBuffer() const { return _cameraBuffer; }
		inline const ObjectBufferData& GetObjectBuffer() const { return _objectBuffer; }
		inline const SunlightBufferData& GetSunlightBuffer() const { return _sunlightBuffer; }
		inline const FogBufferData& GetFogBuffer() const { return _fogBuffer; }
		inline const EnvBufferData& GetEnvBuffer() const { return _envBuffer; };

		const ConfigFile& GetConfig() const { return _config; }

		void UpdateInputTexture(const String& name, BaseTexture* pTexture);
		void UpdateInputSampler(const String& name, Sampler* pSampler);

	protected:

	private:
		struct ShaderPassData
		{
			IShader* pShader;
			Vector<IShaderResource> UsedTextures;
			Vector<IShaderResource> UsedSamplers;
			Vector<IShaderResource> UsedBuffers;
			Map<uint, UniformBuffer*> ConstBuffers;
		};

		void UpdateUniformBuffer(const void* pData, uint binding);
		void ParseShaderFilesConfig(const char* key, ConfigSection* pSection, const String& directoryToAppend, StrMap<String>& shaderPassMap);

		uint SizeFromDataStr(const String& str) const;

		bool RegisterShaderBindings(ShaderBindings* pBindings, uint minBinding, uint maxBindings);

		StrMap<ShaderPassData> _iShaders;
		ConfigFile _config;

		Map<uint, IShaderResource> _constBufferMap;
		Map<uint, IShaderResource> _samplerMap;
		Map<uint, IShaderResource> _textureMap;
		Vector<APIVertexElement> _vertexElements;
		String _activeShaderPass;

		CameraBufferData _cameraBuffer;
		ObjectBufferData _objectBuffer;
		SunlightBufferData _sunlightBuffer;
		FogBufferData _fogBuffer;
		EnvBufferData _envBuffer;

		ShaderBindings _inputTextureBindings;

	};

}