#pragma once

#include "Shader.h"

namespace SunEngine
{
	class DefaultShaders
	{
	public:
		static const String Metallic;
		static const String Specular;
		static const String SkinnedMetallic;
		static const String SkinnedSpecular;
		static const String MetallicAlphaTest;
		static const String SpecularAlphaTest;
		static const String ToneMap;
		static const String Deferred;
		static const String ScreenSpaceReflection;
		static const String SceneCopy;
		static const String FXAA;
		static const String TextureCopy;
		static const String Skybox;
		static const String Clouds;
		static const String SkyArHosek;
		static const String MSAAResolve;

	private:
		DefaultShaders() = delete;
		DefaultShaders(const DefaultShaders&) = delete;
		DefaultShaders& operator= (const DefaultShaders&) = delete;
	};

	class DefaultPipelines
	{
	public:
		static const String ShadowDepth;

	private:
		DefaultPipelines() = delete;
		DefaultPipelines(const DefaultShaders&) = delete;
		DefaultPipelines& operator= (const DefaultPipelines&) = delete;
	};

	class ShaderMgr
	{
	public:
		ShaderMgr(const ShaderMgr&) = delete;
		ShaderMgr& operator = (const ShaderMgr&) = delete;

		static ShaderMgr& Get();

		Shader* GetShader(const String& name) const;
		bool LoadShaders(String& errMsg);

		bool BuildPipelineSettings(const String& pipeline, PipelineSettings& settings) const;
	private:
		struct PipelineField
		{
			uint offset;
			uint size;
			bool isFloat;
		};

		struct PipelineDefinition
		{
			PipelineSettings settings;
			Vector<PipelineField> fields;
		};

		void LoadPipelines();
		void RegisterPipelineSettingsField(const String& field, uint offset, uint size, bool isFloat);

		ShaderMgr();
		~ShaderMgr();

		StrMap<UniquePtr<Shader>> _shaders;
		StrMap<PipelineField> _pipelineFields;
		StrMap<PipelineDefinition> _pipelineDefinitions;
	};
}