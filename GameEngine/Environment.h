#pragma once

#include "AssetNode.h"
#include "SkyModels.h"

namespace SunEngine
{
	class ShaderBindings;
	class TextureCube;

	class EnvironmentComponentData : public ComponentData
	{
	public:
		EnvironmentComponentData(Component* pComponent, SceneNode* pNode);

		float GetDeltaTime() const { return _deltaTime; }
		float GetElapsedTime() const { return _elapsedTime; }

	private:
		friend class Environment;

		float _deltaTime;
		float _elapsedTime;
	};

	class Environment : public Component
	{
	public:
		ComponentType GetType() const { return COMPONENT_ENVIRONMENT; }
		ComponentData* AllocData(SceneNode* pNode) override { return new EnvironmentComponentData(this, pNode); }

		Environment();
		~Environment();

		void Initialize(SceneNode* pNode, ComponentData* pData) override;
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

		void SetSunDirection(const glm::vec3& dir) { _sunDirection = dir; }
		const glm::vec3& GetSunDirection() const { return _sunDirection; }

		bool SetCloudTexture(Texture2D* pCloudTexture);
		ShaderBindings* GetCloudBindings() const { return _cloudtexture ? _cloudBindings.get() : 0; }

		bool SetActiveSkyModel(const String& shaderName);
		SkyModel* GetActiveSkyModel() const;
		SkyModel* GetSkyModel(const String& shaderName) const;
		uint GetSkyModelNames(Vector<String>& names) const;
	private:
		glm::vec3 _sunDirection;
		Texture2D* _cloudtexture;
		UniquePtr<ShaderBindings> _cloudBindings;

		String _activeSkyModel;
		StrMap<UniquePtr<SkyModel>> _skyModels;
	};
}