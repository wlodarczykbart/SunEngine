#pragma once

#include "View.h"
#include "GraphicsPipeline.h"
#include "Shader.h"

namespace SunEngine
{
	class ICameraView : public View
	{
	public:
		virtual ~ICameraView();

		Camera* GetCamera() const { return _camera; }
		CameraComponentData* GetCameraData() const { return _cameraData; }

		virtual bool Render(CommandBuffer* cmdBuffer) = 0;
		virtual void Update(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et);
		virtual void RenderGUI() = 0;
		virtual uint GetGUIColumns() const = 0;

	protected:
		ICameraView(const String& uniqueName);
	private:
		SceneNode _camNode;
		Camera* _camera;
		CameraComponentData* _cameraData;
	};

	class SceneView : public ICameraView
	{
	public:
		SceneView(SceneRenderer* pRenderer);
		~SceneView();

		bool Render(CommandBuffer* cmdBuffer) override;
		void RenderGUI() override;

		uint GetGUIColumns() const override { return 3; }

	private:
		bool OnCreate(const CreateInfo& info) override;
		bool OnResize(const CreateInfo& info) override;

		void BuildSceneTree(SceneNode* pNode);
		void BuildSelectedNodeGUI(Scene* pScene);

		SceneRenderer* _renderer;
		String _selNodeName;
		RenderTarget _sceneTarget;

		Pair<GraphicsPipeline, ShaderBindings> _gammaData;
	};

}