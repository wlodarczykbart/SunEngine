#pragma once

#include "View.h"
#include "GraphicsPipeline.h"
#include "Shader.h"

//#define SUPPORT_SSR

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
		virtual void RenderGUI(GUIRenderer*) = 0;
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
		struct Settings
		{
			Settings();

			struct 
			{
				bool enabled;
				float subpixel;
				float edgeThreshold;
				float edgeThresholdMin;
			} fxaa;

			struct
			{
				bool enabled;
			} msaa;

			struct
			{
				bool enabled;
				float cascadeSplitLambda;
			} shadows;

			struct
			{
				bool showRenderer;
				bool showEnvironments;
			} gui;
		};

		SceneView(SceneRenderer* pRenderer);
		~SceneView();

		bool Render(CommandBuffer* cmdBuffer) override;
		void RenderGUI(GUIRenderer* pRenderer) override;
		uint GetGUIColumns() const override { return 3; }

		Settings& GetSettings() { return _settings; }

		void Update(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et) override;

	private:
		struct RenderPassData
		{
			GraphicsPipeline pipeline;
			ShaderBindings bindings;
			int bufferIndex;
		};

		bool OnCreate(const CreateInfo& info) override;
		bool OnResize(const CreateInfo& info) override;

		void BuildSceneTree(SceneNode* pNode);
		void BuildSelectedNodeGUI(Scene* pScene, GUIRenderer* pRenderer);
		bool CreateRenderPassData(const String& shader, RenderPassData& data, bool useOneZ = false);

		SceneRenderer* _renderer;
		String _selNodeName;
		RenderTarget _opaqueTarget;
		RenderTarget _outputTarget;
		RenderTarget _deferredTarget;
		RenderTarget _deferredResolveTarget;
		RenderTarget _toneMapTarget;
		RenderTarget _msaaTarget;

		RenderPassData _toneMapData;
		RenderPassData _fxaaData;
		RenderPassData _outputData;
		RenderPassData _deferredData;
		RenderPassData _deferredCopyData;
		RenderPassData _msaaResolveData;
#ifdef SUPPORT_SSR
		Pair<GraphicsPipeline, ShaderBindings> _ssrData;
#endif
		UniformBuffer _shaderBuffer;
		int _shaderBufferUsageCount;

		Settings _settings;

		SceneNode* _debugFrustum;

		bool _resizeOccured;
	};

	class ShadowMapView : public View
	{
	public:
		ShadowMapView(SceneRenderer* pSceneRenderer);

	private:
		bool OnCreate(const CreateInfo&) override;
		bool Render(CommandBuffer* cmdBuffer) override;

		ShaderBindings _bindings;
		GraphicsPipeline _pipeline;
		SceneRenderer* _renderer;
	};

}