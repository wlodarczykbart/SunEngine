#pragma once

#include "Types.h"
#include "Scene.h"
#include "PipelineSettings.h"
#include "GraphicsPipeline.h"
#include "UniformBuffer.h"
#include "BaseShader.h"

namespace SunEngine
{
	class CommandBuffer;
	class CameraComponentData;
	class LightComponentData;
	class RenderNode;
	class BaseShader;
	class RenderTarget;

	struct RenderTargetPassInfo
	{
		RenderTarget* pTarget;
		GraphicsPipeline* pPipeline;
		ShaderBindings* pBindings;
	};

	struct DeferredRenderTargetPassInfo : public RenderTargetPassInfo
	{
		RenderTarget* pDeferredResolveTarget;
		GraphicsPipeline* pDeferredCopyPipeline;
		ShaderBindings* pDeferredCopyBindings;
		GraphicsPipeline* pSSRPipeline;
		ShaderBindings* pSSRBindings;
	};

	class SceneRenderer
	{
	public:	
		SceneRenderer();
		~SceneRenderer();

		bool Init();
		bool PrepareFrame(CameraComponentData* pCamera = 0);
		bool RenderFrame(CommandBuffer* cmdBuffer, RenderTarget* pOpaqueTarget, RenderTargetPassInfo* pOutputInfo, DeferredRenderTargetPassInfo* pDeferredInfo);

	private:
		struct UniformBufferData
		{
			uint ArrayIndex;
			uint UpdateIndex;
			UniformBuffer Buffer;
			Map<BaseShader*, ShaderBindings> ShaderBindings;
		};

		struct RenderNodeData
		{
			SceneNode* SceneNode;
			const RenderNode* RenderNode;
			GraphicsPipeline* Pipeline;
			UniformBufferData* ObjectBindings;
			uint ObjectBufferIndex;
		};

		static void TraverseFunc(SceneNode* pNode, void* pUserData);
		void ProcessNode(SceneNode* pNode);
		void ProcessRenderQueue(CommandBuffer* cmdBuffer, Queue<RenderNodeData>& queue);
		GraphicsPipeline* GetPipeline(const RenderNode& node);

		bool _bInit;
		UniquePtr<UniformBufferData> _cameraBuffer;
		UniquePtr<UniformBufferData> _lightBuffer;
		Vector<UniquePtr<GraphicsPipeline>> _graphicsPipelines;
		Vector<UniquePtr<UniformBufferData>> _objectBuffers;
		UniformBufferData* _currentObjectBuffer;
		CameraComponentData* _currentCamera;
		LightComponentData* _currentSunlight;
		HashSet<BaseShader*> _currentShaders;
		Queue<RenderNodeData> _deferredRenderQueue;
		Queue<RenderNodeData> _opaqueRenderQueue;
		Queue<RenderNodeData> _sortedRenderQueue;
		Vector<ObjectBufferData> _objectBufferData;

	};

}