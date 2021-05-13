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

		CameraComponentData(Component* pComponent, SceneNode* pNode) : ComponentData(pComponent, pNode) {}

		bool FrustumIntersects(const AABB& aabb) const;

		glm::mat4 ViewMatrix;
		glm::vec4 FrustumPlanes[6];
		glm::vec3 FrustumCorners[8];
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

		void SetRenderToWindow(bool renderToWindow) { _renderToWindow = renderToWindow; }
		bool GetRenderToWindow() const { return _renderToWindow; }

	private:
		glm::mat4 _projMatrix;
		glm::mat4 _invProjMatrix;
		bool _renderToWindow;
	};
}