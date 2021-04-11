#pragma once

#include "GraphicsAPIDef.h"
#include "glm/gtc/matrix_transform.hpp"

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
			enum ERenderMode
			{
				Forward,
				Deferred,
			};

			GraphicsAPI API() const { return _api; }
			ERenderMode RenderMode() const { return _renderMode; }

			uint CascadeShadowMapResolution() const { return _cascadeShadowMapResolution; }

			uint MaxSkinnedBoneMatrices() const { return _maxSkinnedBoneMatrices; }
			uint MaxTextureTransforms() const { return _maxTextureTransforms; }
			uint MaxShadowCascadeSplits() const { return _maxShadowCascadeSplits; }

			bool ShadowsEnabled() const { return _shadowsEnabled; }

			const glm::mat4& ProjectionCorrection() const { return _projectionCorrection; }

		private:
			friend class EngineInfo;
			Renderer() = default;
			Renderer(const Renderer&) = delete;
			Renderer& operator = (const Renderer&) = delete;

			void Init(ConfigFile* pConfig);

			GraphicsAPI _api;
			glm::mat4 _projectionCorrection;
			ERenderMode _renderMode;
			uint _maxSkinnedBoneMatrices;
			uint _maxTextureTransforms;
			uint _maxShadowCascadeSplits;
			uint _cascadeShadowMapResolution;
			bool _shadowsEnabled;
		};

		class Paths
		{
		public:
			const String& ShaderSourceDir() const;
			const String& ShaderListFile() const;
			const String& Find(const String& key) const;

		private:
			friend class EngineInfo;
			Paths() = default;
			Paths(const Paths&) = delete;
			Paths& operator = (const Paths&) = delete;

			void Init(ConfigFile* pConfig);

			StrMap<String> _paths;
		};

		static const Renderer& GetRenderer()  { return Get()._renderer; }
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