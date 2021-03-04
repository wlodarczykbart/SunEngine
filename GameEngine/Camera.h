#pragma once

#include "AssetNode.h"

namespace SunEngine
{
	enum FrustumType
	{
		FRUSTUM_PERSPECTIVE,
		FRUSTUM_ORTHOGRAPHIC,
	};

	class Frustum
	{
	public:
		void Set(FrustumType type, float left, float right, float bottom, float top, float nearZ, float farZ);

	private:
		FrustumType _type;
		float _left;
		float _right;
		float _top;
		float _bottom;
		float _nearZ;
		float _farZ;
	};

	class CameraComponentData : public ComponentData
	{
	public:
		CameraComponentData(Component* pComponent) : ComponentData(pComponent) {}

		glm::mat4 ViewMatrix;
	};

	class Camera : public Component
	{
	public:
		Camera();
		~Camera();

		ComponentType GetType() const override { return COMPONENT_CAMERA; }
		ComponentData* AllocData() override { return new CameraComponentData(this); }

		void SetFrustum(FrustumType type, float left, float right, float bottom, float top, float nearZ, float farZ);
		void SetFrustum(float fovAngle, float aspectRatio, float nearZ, float farZ);
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

		const glm::mat4& GetProj() const { return _projMatrix; }

		void SetRenderToWindow(bool renderToWindow) { _renderToWindow = renderToWindow; }
		bool GetRenderToWindow() const { return _renderToWindow; }

	private:
		Frustum _frustum;
		glm::mat4 _projMatrix;
		bool _renderToWindow;
	};
}