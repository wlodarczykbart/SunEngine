#include "ArHosekSkyModel.h"
#include "GraphicsWindow.h"

#include "ShaderMgr.h"
#include "SkyModels.h"

namespace SunEngine
{
	SkyModel::SkyModel()
	{
		_material = UniquePtr<Material>(new Material());
	}

	SkyModel::~SkyModel()
	{

	}

	SkyModelSkybox::SkyModelSkybox()
	{
		_skybox = 0;
	}

	void SkyModelSkybox::Init()
	{
		Shader* pShader = ShaderMgr::Get().GetShader(DefaultShaders::Skybox);
		_material->SetShader(pShader);
		_material->RegisterToGPU();
	}

	bool SkyModelSkybox::SetSkybox(TextureCube* pSkybox)
	{
		if (_skybox != pSkybox)
		{
			if (pSkybox)
			{
				if (!_material->SetTextureCube(MaterialStrings::DiffuseMap, pSkybox))
					return false;
			}

			_skybox = pSkybox;
			return true;
		}
		else
		{
			return true;
		}
	}

	SkyModelArHosek::SkyModelArHosek()
	{
		_turbidity = 2.0f;
		_albedo = 0.5f;
		_intensity = 3.0f;
	}

	void SkyModelArHosek::Init()
	{
		Shader* pShader = ShaderMgr::Get().GetShader(DefaultShaders::SkyArHosek);
		_material->SetShader(pShader);
		_material->RegisterToGPU();	

		_sampleDirections.clear();
		for (uint i = 0; i < 64; i++)
		{
			_sampleDirections.push_back(glm::normalize(glm::linearRand(glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.1f, 1.0f))));
		}
	}

	void SkyModelArHosek::Update(const glm::vec3& sunDirection)
	{
		ArHosekSkyModelState* skymodel_state = NULL;

		glm::vec3 sunDir = glm::normalize(sunDirection);
		float thetaS = glm::acos(glm::max(glm::dot(sunDir, glm::vec3(0, 1, 0)), 0.00001f));
		float elevation = glm::half_pi<float>() - thetaS;

		skymodel_state =
			arhosek_rgb_skymodelstate_alloc_init(
				_turbidity,
				_albedo,
				elevation
			);

		const uint NUM_CONFIG = sizeof(ArHosekSkyModelConfiguration) / sizeof(double);

		glm::mat4 StateConfigR;
		glm::mat4 StateConfigG;
		glm::mat4 StateConfigB;
		glm::vec4 StateRadiances;

		float* StatesArray[3] =
		{
			&StateConfigR[0][0],
			&StateConfigG[0][0],
			&StateConfigB[0][0],
		};

		for (uint i = 0; i < 3; i++)
		{
			StateRadiances[i] = (float)skymodel_state->radiances[i];
			for (uint j = 0; j < NUM_CONFIG; j++)
				StatesArray[i][j] = (float)skymodel_state->configs[i][j];
		}
		StateRadiances.a = _intensity;

		_material->SetMaterialVar("StateConfigR", StateConfigR);
		_material->SetMaterialVar("StateConfigG", StateConfigG);
		_material->SetMaterialVar("StateConfigB", StateConfigB);
		_material->SetMaterialVar("StateRadiances", StateRadiances);


		_skyColor = Vec3::Zero;
		for (uint i = 0; i < _sampleDirections.size(); i++)
		{
			float VdotS = glm::dot(_sampleDirections[i], sunDir);
			float gamma = acos(glm::min(1.0f, glm::max(VdotS, 0.00001f)));
			float theta = acos(glm::min(1.0f, glm::max(glm::dot(_sampleDirections[i], Vec3::Up), 0.00001f)));

			glm::vec3 color;
			color.r = (float)arhosek_tristim_skymodel_radiance(skymodel_state, theta, gamma, 0);
			color.g = (float)arhosek_tristim_skymodel_radiance(skymodel_state, theta, gamma, 1);
			color.b = (float)arhosek_tristim_skymodel_radiance(skymodel_state, theta, gamma, 2);
			color /= 255.0f;
			color = glm::pow(color, glm::vec3(1.0f / 2.2f));
			
			_skyColor += color;
		}
		_skyColor /= _sampleDirections.size();
		_skyColor *= _intensity;

		arhosekskymodelstate_free(skymodel_state);
	}
}