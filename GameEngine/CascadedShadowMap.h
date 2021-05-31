#pragma once

#include "MathHelper.h"

namespace SunEngine
{
	class CameraComponentData;
	class Camera;

	class CascadedShadowMap
	{
	public:		
		enum CascadeFitMode
		{
			FIT_TO_CASCADES,
			FIT_TO_SCENE,
		};

		enum NearFarFitMode
		{
			FIT_NEARFAR_AABB,
			FIT_NEARFAR_SCENE_AABB,
			FIT_NEARFAR_PANCAKING,
		};

		struct UpdateInfo
		{
			CameraComponentData* pCameraData;
			AABB sceneBounds;
			glm::vec3 lightPos;
			float cascadeSplitLambda;
			CascadeFitMode cascadeFitMode;
			NearFarFitMode nearFarFitMode;
		};

		static void Update(const UpdateInfo& updateInfo, glm::mat4& lightViewMatrix, Camera* pShadowCameras, float* pShadowFrustumParitions);
	private:
		CascadedShadowMap();
		CascadedShadowMap(const CascadedShadowMap&) = delete;
		CascadedShadowMap& operator = (const CascadedShadowMap&) = delete;
		~CascadedShadowMap();

	};
}