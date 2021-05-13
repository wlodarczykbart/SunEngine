#pragma once

#include "AssetNode.h"

namespace SunEngine
{
	enum LightType
	{
		LT_DIRECTIONAL,
		LT_POINT,
		LT_SPOT,
	};

	class LightComponentData : public ComponentData
	{
	public:
		LightComponentData(Component* pComponent, SceneNode* pNode) : ComponentData(pComponent, pNode) {}

		glm::vec4 Direction;
	};

	class Light : public Component
	{
	public:
		ComponentType GetType() const { return COMPONENT_LIGHT; }
		ComponentData* AllocData(SceneNode* pNode) override { return new LightComponentData(this, pNode); }

		Light();
		~Light();

		void SetLightType(LightType type) { _type = type; }
		LightType GetLightType() const { return _type; }

		void SetColor(const glm::vec4& color) { _color = color; }
		const glm::vec4& GetColor() const { return _color; }

		void Initialize(SceneNode* pNode, ComponentData* pData) override;
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

	private:
		LightType _type;
		glm::vec4 _color;
		glm::vec4 _direction;
	};
}