#include "SceneNode.h"
#include "Scene.h"
#include "ShaderMgr.h"
#include "Material.h"
#include "TextureCube.h"
#include "ResourceMgr.h"
#include "Environment.h"

namespace SunEngine
{
	EnvironmentComponentData::EnvironmentComponentData(Component* pComponent, SceneNode* pNode) : ComponentData(pComponent, pNode)
	{
		_deltaTime = 0.0f;
		_elapsedTime = 0.0f;
	}

	Environment::Environment()
	{
		_sunDirection = glm::normalize(glm::vec3(1, 1, 1));
		_cloudtexture = 0;
		_cloudBindings = UniquePtr<ShaderBindings>(new ShaderBindings());

		_skyModels[DefaultShaders::Skybox] = UniquePtr<SkyModel>(new SkyModelSkybox());
		_skyModels[DefaultShaders::SkyArHosek] = UniquePtr<SkyModel>(new SkyModelArHosek());

		_activeSkyModel = DefaultShaders::SkyArHosek;
	}

	Environment::~Environment()
	{

	}

	void Environment::Initialize(SceneNode* pNode, ComponentData* pData)
	{
		pNode->GetScene()->RegisterEnvironment(pData->As<EnvironmentComponentData>());

		for (auto& sky : _skyModels)
			sky.second->Init();
	}

	void Environment::Update(SceneNode*, ComponentData* pData, float dt, float et)
	{
		auto* envData = pData->As<EnvironmentComponentData>();
		envData->_deltaTime = dt;
		envData->_elapsedTime = et;

		for (auto& sky : _skyModels)
			sky.second->Update(_sunDirection);
	}

	bool Environment::SetCloudTexture(Texture2D* pCloudTexture)
	{
		if (_cloudtexture != pCloudTexture)
		{
			if (pCloudTexture)
			{
				//probably dont need to create this everytime that this is set again...but doing it since that is eaiest solution for now
				Shader* pShader = ShaderMgr::Get().GetShader(DefaultShaders::Clouds);
				if (!pShader)
					return false;

				ShaderBindings::CreateInfo info = {};
				info.type = SBT_MATERIAL;
				info.pShader = pShader->GetDefault();
				if (!_cloudBindings->Create(info))
					return false;
				if (!_cloudBindings->SetTexture(MaterialStrings::DiffuseMap, pCloudTexture->GetGPUObject()))
					return false;
				if (!_cloudBindings->SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_REPEAT)))
					return false;
			}

			_cloudtexture = pCloudTexture;
			return true;
		}
		else
		{
			return true;
		}
	}

	bool Environment::SetActiveSkyModel(const String& shaderName)
	{
		auto found = _skyModels.find(shaderName);
		if (found != _skyModels.end())
		{
			_activeSkyModel = shaderName;
			return true;
		}
		else
		{
			return false;
		}
	}

	SkyModel* Environment::GetActiveSkyModel() const
	{
		return _skyModels.at(_activeSkyModel).get();
	}

	SkyModel* Environment::GetSkyModel(const String& shaderName) const
	{
		auto found = _skyModels.find(shaderName);
		if (found != _skyModels.end())
		{
			return (*found).second.get();
		}
		else
		{
			return 0;
		}
	}

	uint Environment::GetSkyModelNames(Vector<String>& names) const
	{
		for (auto& sky : _skyModels)
			names.push_back(sky.first);

		return _skyModels.size();
	}
}