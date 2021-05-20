#include "SceneNode.h"
#include "Scene.h"
#include "Light.h"

namespace SunEngine
{
	Light::Light()
	{
		SetLightType(LT_SPOT);
		SetColor(glm::vec4(1));
	}

	Light::~Light()
	{
	}

	void Light::Initialize(SceneNode* pNode, ComponentData* pData)
	{
		pNode->GetScene()->RegisterLight(pData->As<LightComponentData>());
	}

	void Light::Update(SceneNode* pNode, ComponentData* pData, float, float)
	{
		pData->As<LightComponentData>()->Direction = pNode->GetWorld()[2];
	}
}