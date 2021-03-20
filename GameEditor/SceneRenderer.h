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
	class Shader;

	class SceneRenderer
	{
	public:	
		SceneRenderer();
		~SceneRenderer();

		bool Init();
		bool PrepareFrame(CameraComponentData* pCamera = 0);
		bool RenderFrame(CommandBuffer* cmdBuffer);

	private:
		struct UniformBufferData
		{
			uint ArrayIndex;
			uint UpdateIndex;
			UniformBuffer Buffer;
			Map<Shader*, ShaderBindings> ShaderBindings;
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
		GraphicsPipeline* GetPipeline(const RenderNode& node);

		bool _bInit;
		UniquePtr<UniformBufferData> _cameraBuffer;
		UniquePtr<UniformBufferData> _lightBuffer;
		Vector<UniquePtr<GraphicsPipeline>> _graphicsPipelines;
		Vector<UniquePtr<UniformBufferData>> _objectBuffers;
		UniformBufferData* _currentObjectBuffer;
		CameraComponentData* _currentCamera;
		LightComponentData* _currentSunlight;
		HashSet<Shader*> _currentShaders;
		Queue<RenderNodeData> _renderQueue;
		Vector<ObjectBufferData> _objectBufferData;

	};

}