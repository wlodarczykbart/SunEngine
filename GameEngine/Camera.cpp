#include "SceneNode.h"
#include "Camera.h"

namespace SunEngine
{
	Camera::Camera()
	{
		_renderToWindow = false;
		_projMatrix = glm::mat4(1.0f);
	}

	Camera::~Camera()
	{
	}

	void Camera::SetFrustum(FrustumType type, float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		_frustum.Set(type, left, right, bottom, top, nearZ, farZ);

		switch (type)
		{
		case SunEngine::FRUSTUM_PERSPECTIVE:
			_projMatrix = glm::frustum(left, right, bottom, top, nearZ, farZ);
			break;
		case SunEngine::FRUSTUM_ORTHOGRAPHIC:
			_projMatrix = glm::ortho(left, right, bottom, top, nearZ, farZ);
			break;
		default:
			break;
		}
	}

	void Camera::SetFrustum(float fovAngle, float aspectRatio, float nearZ, float farZ)
	{
		float t = tanf(glm::radians(fovAngle) * 0.5f) * nearZ;
		float width = t * aspectRatio;
		float height = t;

		//glm::mat4 m0 = glm::perspective(fovAngle, aspectRatio, nearZ, farZ);
		//glm::mat4 m1 = glm::frustum(-width, +width, -height, +height, nearZ, farZ);
		//bool eq = m0 == m1;

		SetFrustum(FRUSTUM_PERSPECTIVE, -width, +width, -height, +height, nearZ, farZ);
	}

	void Camera::Update(SceneNode* pNode, ComponentData* pData, float, float)
	{
		CameraComponentData* pCamData = static_cast<CameraComponentData*>(pData);
		pCamData->ViewMatrix = glm::inverse(pNode->GetWorld());
	}

	void Frustum::Set(FrustumType type, float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		_type = type;
		_left = left;
		_right = right;
		_bottom = bottom;
		_top = top;
		_nearZ = nearZ;
		_farZ = farZ;
	}
}