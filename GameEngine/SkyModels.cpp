#include "ArHosekSkyModel.h"
#include "glm/gtc/constants.hpp"

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

	void SkyModelArHosek::Init()
	{
		const uint texSize = 512;
		const float phiStep = glm::pi<float>() / (texSize - 1);
		const float thetaStep = glm::pi<float>() / (texSize - 1);

		//generate a texture that we lookup to get the spherical information needed by skymodel to avoid calling trig functions in pixel shader
		_sphericalLookupTexture.Alloc(texSize, texSize);

		for (uint y = 0; y < texSize; y++)
		{
			float phi = y * phiStep;
			for (uint x = 0; x < texSize; x++)
			{
				float theta = x * thetaStep;

				glm::vec4 dir;
				//spherical coordinates
				dir.x = sinf(phi) * cosf(theta);
				dir.y = sinf(phi) * sinf(theta);
				dir.z = cosf(phi);
				//theta
				dir.w = atan2(dir.y, dir.x);

				dir = dir * 0.5f + 0.5f;
				_sphericalLookupTexture.SetPixel(y, texSize - 1 - x, dir);
			}
		}

		_sphericalLookupTexture.RegisterToGPU();

		Shader* pShader = ShaderMgr::Get().GetShader(DefaultShaders::SkyArHosek);
		_material->SetShader(pShader);
		_material->RegisterToGPU();	
		_material->SetTexture2D(MaterialStrings::DiffuseMap, &_sphericalLookupTexture);
	}

	void SkyModelArHosek::Update(const glm::vec3& sunDirection)
	{
		ArHosekSkyModelState* skymodel_state = NULL;

		const float albedo = 3.2f;
		const float turbidity = 2.0f;

		glm::vec3 sunDir = glm::normalize(sunDirection);
		float thetaS = acosf(glm::dot(sunDir, glm::vec3(0, 1, 0)));
		float elevation = glm::half_pi<float>() - thetaS;

		skymodel_state =
			arhosek_rgb_skymodelstate_alloc_init(
				turbidity,
				albedo,
				elevation
			);

		const uint NUM_CONFIG = sizeof(ArHosekSkyModelConfiguration) / sizeof(double);

		float state_configs[3][NUM_CONFIG];
		glm::vec4 state_radiances;

		for (uint i = 0; i < 3; i++)
		{
			for (uint j = 0; j < NUM_CONFIG; j++)
				state_configs[i][j] = (float)skymodel_state->configs[i][j];
			state_radiances[i] = (float)skymodel_state->radiances[i];
		}

		_material->SetMaterialVar("StateConfigR", state_configs[0], sizeof(float) * NUM_CONFIG);
		_material->SetMaterialVar("StateConfigG", state_configs[1], sizeof(float) * NUM_CONFIG);
		_material->SetMaterialVar("StateConfigB", state_configs[2], sizeof(float) * NUM_CONFIG);
		_material->SetMaterialVar("StateRadiances", state_radiances);

		arhosekskymodelstate_free(skymodel_state);
	}
}