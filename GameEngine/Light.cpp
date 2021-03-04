#include "SceneNode.h"

#include "Light.h"

namespace SunEngine
{
	Light::Light()
	{
		SetLightType(LT_DIRECTIONAL);
		SetColor(glm::vec4(1));
	}

	Light::~Light()
	{
	}

	void Light::Update(SceneNode* pNode, ComponentData* pData, float, float)
	{
		static_cast<LightComponentData*>(pData)->Direction = pNode->GetWorld()[2];
	}
}