#pragma once

#include "GraphicsAPIDef.h"

namespace SunEngine
{
	class ConfigFile;

	class EngineInfo
	{
	public:
		static void Init(ConfigFile* pConfig);

		class Renderer
		{
		public:
			class Limits
			{
			public:
				static const uint MinCascadeShadowMapResolution = 512;
				static const uint MaxCascadeShadowMapResolution = 4096;
				static const uint MinCascadeShadowPCFBlurSize = 1;
				static const uint MaxCascadeShadowPCFBlurSize = 5;
				static const uint MinCascadeShadowMapSplits = 1;
				static const uint MaxCascadeShadowMapSplits = 4;
				static const uint MinSkinnedBoneMatrices = 64;
				static const uint MaxSkinnedBoneMatrices = 256;
				static const uint MinTerrainTextures = 4;
				static const uint MaxTerrainTextures = 16;
				static const uint MinTerrainTextureResolution = 512;
				static const uint MaxTerrainTextureResolution = 2048;
			};

			enum ERenderMode
			{
				Forward,
				Deferred,
			};

			GraphicsAPI API() const { return _api; }
			ERenderMode RenderMode() const { return _renderMode; }

			uint CascadeShadowMapResolution() const { return _cascadeShadowMapResolution; }
			uint CascadeShadowMapPCFBlurSize() const { return _cascadeShadowMapPCFBlurSize; }
			uint CascadeShadowMapSplits() const { return _cascadeShadowMapSplits; }

			uint SkinnedBoneMatrices() const { return _skinnedBoneMatrices; }
			uint TerrainTextures() const { return _terrainTextures; }
			uint TerrainTextureResolution() const { return _terrainTextureResolution; }

			bool ShadowsEnabled() const { return _shadowsEnabled; }
			MSAAMode GetMSAAMode() const { return _msaaMode; }

			void SetRenderMode(ERenderMode mode) { _renderMode = mode; }

		private:
			friend class EngineInfo;
			Renderer() = default;
			Renderer(const Renderer&) = delete;
			Renderer& operator = (const Renderer&) = delete;

			void Init(ConfigFile* pConfig);

			GraphicsAPI _api;
			ERenderMode _renderMode;
			uint _skinnedBoneMatrices;
			uint _cascadeShadowMapSplits;
			uint _cascadeShadowMapResolution;
			uint _cascadeShadowMapPCFBlurSize;
			bool _shadowsEnabled;
			MSAAMode _msaaMode;
			uint _terrainTextures;
			uint _terrainTextureResolution;
		};

		class Paths
		{
		public:
			const String& ShaderSourceDir() const;
			const String& ShaderListFile() const;
			const String& ShaderPipelineListFile() const;
			const String& Find(const String& key) const;

		private:
			friend class EngineInfo;
			Paths() = default;
			Paths(const Paths&) = delete;
			Paths& operator = (const Paths&) = delete;

			void Init(ConfigFile* pConfig);

			StrMap<String> _paths;
		};

		static Renderer& GetRenderer() { return Get()._renderer; }
		static const Paths& GetPaths() { return Get()._paths; }

	private:
		static EngineInfo& Get();

		EngineInfo() = default;
		EngineInfo(const EngineInfo&) = delete;
		EngineInfo& operator = (const EngineInfo&) = delete;
		~EngineInfo() = default;

		Renderer _renderer;
		Paths _paths;
		
	};
}