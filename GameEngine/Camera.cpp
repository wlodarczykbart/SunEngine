#include "SceneNode.h"
#include "Scene.h"
#include "FilePathMgr.h"
#include "Camera.h"

namespace SunEngine
{
	Camera::Camera()
	{
		_renderToWindow = false;
		_projMatrix = glm::mat4(1.0f);
		_invProjMatrix = glm::mat4(1.0f);
	}

	Camera::~Camera()
	{
	}

	void Camera::SetProjection(float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		_projMatrix = glm::frustumRH_ZO(left, right, bottom, top, nearZ, farZ);
		_invProjMatrix = glm::inverse(_projMatrix);
	}

	void Camera::SetOrthoProjection(float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		_projMatrix = glm::orthoRH_ZO(left, right, bottom, top, nearZ, farZ);
		_invProjMatrix = glm::inverse(_projMatrix);
	}

	void Camera::SetProjection(float fovAngle, float aspectRatio, float nearZ, float farZ)
	{
		_projMatrix = glm::perspective(fovAngle, aspectRatio, nearZ, farZ);
		_invProjMatrix = glm::inverse(_projMatrix);
	}

	void Camera::Initialize(SceneNode* pNode, ComponentData* pData)
	{
		pNode->GetScene()->RegisterCamera(pData->As<CameraComponentData>());
	}

	void Camera::Update(SceneNode* pNode, ComponentData* pData, float, float)
	{
		CameraComponentData* pCamData = pData->As<CameraComponentData>();

		glm::mat4 camWorld = pNode->GetWorld();

		//remove scaling
		camWorld[0] = glm::normalize(camWorld[0]);
		camWorld[1] = glm::normalize(camWorld[1]);
		camWorld[2] = glm::normalize(camWorld[2]);

		pCamData->_viewMatrix = glm::inverse(camWorld);
		pCamData->_right = camWorld[0];
		pCamData->_up = camWorld[1];
		pCamData->_forward = -camWorld[2];
		pCamData->_position = camWorld[3];

		glm::vec3* corners = pCamData->_frustumCorners;
		corners[CameraComponentData::LBN] = glm::vec3(-1.0f, -1.0f, +0.0f);
		corners[CameraComponentData::RBN] = glm::vec3(+1.0f, -1.0f, +0.0f);
		corners[CameraComponentData::LTN] = glm::vec3(-1.0f, +1.0f, +0.0f);
		corners[CameraComponentData::RTN] = glm::vec3(+1.0f, +1.0f, +0.0f);
		corners[CameraComponentData::LBF] = glm::vec3(-1.0f, -1.0f, +1.0f);
		corners[CameraComponentData::RBF] = glm::vec3(+1.0f, -1.0f, +1.0f);
		corners[CameraComponentData::LTF] = glm::vec3(-1.0f, +1.0f, +1.0f);
		corners[CameraComponentData::RTF] = glm::vec3(+1.0f, +1.0f, +1.0f);

		for (uint i = 0; i < 8; i++)
		{
			glm::vec4 viewPos = _invProjMatrix * glm::vec4(corners[i], 1.0f);
			corners[i] = camWorld * (viewPos / viewPos.w);
		}

#if 0
		FrustumType type;
		float l, r, b, t, n, f;
		_frustum.Get(type, l, r, b, t, n, f);

		glm::vec3 right = glm::normalize(camWorld[0]);
		glm::vec3 up = glm::normalize(camWorld[1]);
		glm::vec3 fwd = glm::normalize(camWorld[2]);
		glm::vec3 pos = camWorld[3];

		float ln = l;
		float rn = r;
		float bn = b;
		float tn = t;

		l /= n;
		r /= n;
		b /= n;
		t /= n;

		float lf = l * f;
		float rf = r * f;
		float bf = b * f;
		float tf = t * f;

		glm::vec3 nearPos = pos + fwd * -n;
		glm::vec3 farPos = pos + fwd * -f;

		float vScale = 1.0f;

		corners[CameraComponentData::LBN] = (right * ln * vScale) + (up * bn * vScale) + nearPos;
		corners[CameraComponentData::RBN] = (right * rn * vScale) + (up * bn * vScale) + nearPos;
		corners[CameraComponentData::LTN] = (right * ln * vScale) + (up * tn * vScale) + nearPos;
		corners[CameraComponentData::RTN] = (right * rn * vScale) + (up * tn * vScale) + nearPos;

		corners[CameraComponentData::LBF] = (right * lf * vScale) + (up * bf * vScale) + farPos;
		corners[CameraComponentData::RBF] = (right * rf * vScale) + (up * bf * vScale) + farPos;
		corners[CameraComponentData::LTF] = (right * lf * vScale) + (up * tf * vScale) + farPos;
		corners[CameraComponentData::RTF] = (right * rf * vScale) + (up * tf * vScale) + farPos;

		for (uint i = 0; i < 8; i++)
		{
			auto equal = glm::equal(tmp[i], corners[i], 0.1f);
			assert(equal[0]);
			assert(equal[1]);
			assert(equal[2]);
		}
#endif
	
		pCamData->_frustumPlanes[CameraComponentData::LEFT] = CreatePlane(corners[CameraComponentData::LBN], corners[CameraComponentData::LTF], corners[CameraComponentData::LBF]);
		pCamData->_frustumPlanes[CameraComponentData::RIGHT] = CreatePlane(corners[CameraComponentData::RBN], corners[CameraComponentData::RBF], corners[CameraComponentData::RTF]);
		pCamData->_frustumPlanes[CameraComponentData::BOTTOM] = CreatePlane(corners[CameraComponentData::LBF], corners[CameraComponentData::RBF], corners[CameraComponentData::RBN]);
		pCamData->_frustumPlanes[CameraComponentData::TOP] = CreatePlane(corners[CameraComponentData::LTN], corners[CameraComponentData::RTN], corners[CameraComponentData::RTF]);
		pCamData->_frustumPlanes[CameraComponentData::NEAR] = CreatePlane(corners[CameraComponentData::LBN], corners[CameraComponentData::RBN], corners[CameraComponentData::RTN]);
		pCamData->_frustumPlanes[CameraComponentData::FAR] = CreatePlane(corners[CameraComponentData::RBF], corners[CameraComponentData::LBF], corners[CameraComponentData::LTF]);
	}

	CameraComponentData::CameraComponentData(Component* pComponent, SceneNode* pNode) : ComponentData(pComponent, pNode)
	{
		_viewMatrix = glm::mat4(1.0f);
		_position = glm::vec3(0.0f);
		_right = glm::vec3(1.0f, 0.0f, 0.0f);
		_up = glm::vec3(0.0f, 1.0f, 0.0f);
		_forward = glm::vec3(0.0f, 0.0f, -1.0f);
	}

	bool CameraComponentData::FrustumIntersects(const AABB& aabb) const
	{
		//return true;

		glm::vec4 corners[8];
		aabb.GetCorners(corners);

		for (uint i = 0; i < 6; i++)
		{
			//if any point is inside the plane, the plane test is passed for this plane against the cube
			if (glm::dot(corners[0], _frustumPlanes[i]) < 0.0f) continue;
			if (glm::dot(corners[1], _frustumPlanes[i]) < 0.0f) continue;
			if (glm::dot(corners[2], _frustumPlanes[i]) < 0.0f) continue;
			if (glm::dot(corners[3], _frustumPlanes[i]) < 0.0f) continue;
			if (glm::dot(corners[4], _frustumPlanes[i]) < 0.0f) continue;
			if (glm::dot(corners[5], _frustumPlanes[i]) < 0.0f) continue;
			if (glm::dot(corners[6], _frustumPlanes[i]) < 0.0f) continue;
			if (glm::dot(corners[7], _frustumPlanes[i]) < 0.0f) continue;
				
			//all points of the cube are outside the plane, so the cube is outside the frustum
			return false;
		}
		return true;
	}
}