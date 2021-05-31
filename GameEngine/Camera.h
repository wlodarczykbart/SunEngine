#pragma once

#include "AssetNode.h"

namespace SunEngine
{
	class CameraComponentData : public ComponentData
	{
	public:
		enum FrustumPlane
		{
			LEFT,
			RIGHT,
			BOTTOM,
			TOP,
			NEAR,
			FAR,
		};

		enum FrustumCorner
		{
			LBN,
			RBN,
			LTN,
			RTN,
			LBF,
			RBF,
			LTF,
			RTF,
		};

		CameraComponentData();
		CameraComponentData(Component* pComponent, SceneNode* pNode);

		bool FrustumIntersects(const AABB& aabb) const;

		const glm::mat4& GetView() const { return _viewMatrix; }
		const glm::mat4& GetInvView() const { return _invViewMatrix; }
		const glm::vec3& GetPosition() const { return _position; }
		const glm::vec3& GetRight() const { return _right; }
		const glm::vec3& GetUp() const { return _up; }
		const glm::vec3& GetForward() const { return _forward; }

		const glm::vec3& GetFrustumCorner(uint index) const { return _frustumCorners[index]; }
					  
	private:
		friend class Camera;
		glm::mat4 _viewMatrix;
		glm::mat4 _invViewMatrix;
		glm::vec3 _position;
		glm::vec3 _right;
		glm::vec3 _up;
		glm::vec3 _forward;
		glm::vec4 _frustumPlanes[6];
		glm::vec3 _frustumCorners[8];
	};

	class Camera : public Component
	{
	public:
		Camera();
		~Camera();

		ComponentType GetType() const override { return COMPONENT_CAMERA; }
		ComponentData* AllocData(SceneNode* pNode) override { return new CameraComponentData(this, pNode); }

		void SetProjection(float left, float right, float bottom, float top, float nearZ, float farZ);
		void SetOrthoProjection(float left, float right, float bottom, float top, float nearZ, float farZ);
		void SetProjection(float fovAngle, float aspectRatio, float nearZ, float farZ);
		void Initialize(SceneNode* pNode, ComponentData* pData) override;
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

		const glm::mat4& GetProj() const { return _projMatrix; }
		const glm::mat4& GetInvProj() const { return _invProjMatrix; }

		float GetNearZ() const { return _nearZ; }
		float GetFarZ() const { return _farZ; }

	private:
		glm::mat4 _projMatrix;
		glm::mat4 _invProjMatrix;
		float _nearZ;
		float _farZ;
	};
}