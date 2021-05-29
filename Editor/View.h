#pragma once

#include "RenderTarget.h"
#include "InputCodes.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace SunEngine
{
	struct GWEventData;
	class SceneRenderer;
	class GraphicsWindow;
	class GUIRenderer;

	class View
	{
	public:
		enum CameraMode
		{
			CM_STATIC,
			CM_FIRST_PERSON,
			CM_CUSTOM,
		};

		struct CreateInfo
		{
			uint width;
			uint height;
			bool visible;
			bool floatingPointColorBuffer;

			bool ParseConfigString(const String& str);
		};

		virtual ~View();

		const String& GetName() const { return _name; }
		RenderTarget* GetRenderTarget() { return &_target; }
		glm::vec2 GetSize() const { return _viewSize; }
		glm::vec2 GetPos() const { return _viewPos; }
		bool IsFocused() const { return _viewFocused; }
		bool IsMouseInside() const { return _mouseInsideView; }
		glm::vec2 GetRelativeMousPos() const { return _relativeMousePosition; }

		void GetTransform(glm::vec3& position, glm::quat& orientation) const;
		float GetFOV() const { return _fovAngle; }
		float GetAspectRatio() const { return _aspectRatio; }
		float GetNearZ() const { return _nearZ; }
		float GetFarZ() const { return _farZ; }

		const glm::mat4& GetWorldMatrix() const { return _worldMtx; }
		const glm::mat4& GetViewMatrix() const { return _viewMtx; }
		const glm::mat4& GetProjMatrix() const { return _projMtx; }
		 
		bool Create(const CreateInfo& info);

		void UpdateViewState(const glm::vec2& viewSize, const glm::vec2& viewPos, bool mouseInsideView, bool isFocused);

		virtual bool Render(CommandBuffer* cmdBuffer);
		virtual void Update(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et);
		virtual void RenderGUI(GUIRenderer* pRenderer) { (void)pRenderer; };
		virtual uint GetGUIColumns() const { return 1; }

		bool NeedsResize() const { return _needsResize; }

		void SetRenderToGraphicsWindow(bool bRender) { _renderToGraphicsWindow = bRender; }
		bool GetRenderToGraphicsWindow() const { return _renderToGraphicsWindow; }

		bool GetVisible() const { return _info.visible; }
		void SetVisible(bool visible) { _info.visible = visible; }

		bool GeCameraMode() const { return _camMode; }
		void SetCameraMode(CameraMode mode) { _camMode = mode; }

	protected:
		View(const String& uniqueName);

		virtual bool OnCreate(const CreateInfo&) { return true; };
		virtual bool OnResize(const CreateInfo&) { return true; };

		RenderTarget _target;
	private:
		void UpdateFirstPersonCamera(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et);

		CreateInfo _info;

		String _name;

		CameraMode _camMode;
		glm::vec3 _position;
		glm::vec3 _rotation;
		glm::mat4 _worldMtx;
		glm::mat4 _viewMtx;

		float _fovAngle;
		float _aspectRatio;
		float _nearZ;
		float _farZ;
		glm::mat4 _projMtx;

		bool _mouseButtonStates[MOUSE_BUTTON_COUNT];
		glm::vec2 _mousePositions[MOUSE_BUTTON_COUNT];
		glm::vec2 _viewSize;
		glm::vec2 _viewPos;
		bool _viewFocused;
		bool _needsResize;
		bool _mouseInsideView;
		bool _renderToGraphicsWindow;

		glm::vec2 _relativeMousePosition;
	};

}