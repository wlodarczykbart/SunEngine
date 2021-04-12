#pragma once

#include "Types.h"
#include "Scene.h"
#include "PipelineSettings.h"
#include "GraphicsPipeline.h"
#include "UniformBuffer.h"
#include "BaseShader.h"
#include "RenderTarget.h"

namespace SunEngine
{
	class CommandBuffer;
	class CameraComponentData;
	class LightComponentData;
	class RenderNode;
	class BaseShader;
	class RenderTarget;
	class Material;

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

		template<typename T>
		class UniformBufferGroup
		{
		public:
			UniformBufferGroup();
			bool Init();
			void Flush();
			void Reset();
			void Update(const T& dataBlock, uint& updatedIndex, UniformBufferData** ppUpdatedBuffer = 0, BaseShader* pShader = 0);

		private:
			Vector<UniquePtr<UniformBufferData>> _buffers;
			UniformBufferData* _current;
			Vector<T> _data;
		};

		struct RenderNodeData
		{
			SceneNode* SceneNode;
			const RenderNode* RenderNode;
			GraphicsPipeline* Pipeline;
			UniformBufferData* ObjectBindings;
			uint ObjectBufferIndex;
			Material* MaterialOverride;
		};

		struct DepthRenderData
		{
			UniformBufferGroup<ObjectBufferData> ObjectBufferGroup;
			Queue<RenderNodeData> RenderQueue;
			Viewport Viewport;
		};

		static void TraverseFunc(SceneNode* data, void* pUserData);
		void ProcessNode(SceneNode* pNode);
		void ProcessRenderQueue(CommandBuffer* cmdBuffer, Queue<RenderNodeData>& queue, uint cameraUpdateIndex);
		bool GetPipeline(RenderNodeData& node);
		void TryBindBuffer(CommandBuffer* cmdBuffer, BaseShader* pShader, UniformBufferData* buffer, IBindState* pBindState = 0) const;

		bool _bInit;
		UniformBufferGroup<CameraBufferData> _cameraGroup;
		UniformBufferData* _cameraBuffer;
		UniquePtr<UniformBufferData> _lightBuffer;
		UniquePtr<UniformBufferData> _shadowMatrixBuffer;
		Vector<UniquePtr<GraphicsPipeline>> _graphicsPipelines;
		UniformBufferGroup<ObjectBufferData> _objectBufferGroup;
		CameraComponentData* _currentCamera;
		LightComponentData* _currentSunlight;
		HashSet<BaseShader*> _currentShaders;
		Queue<RenderNodeData> _gbufferRenderQueue;
		Queue<RenderNodeData> _opaqueRenderQueue;
		Queue<RenderNodeData> _sortedRenderQueue;

		Map<usize, UniquePtr<Material>> _depthMaterials;
		RenderTarget _depthTarget;
		Vector<UniquePtr<DepthRenderData>> _depthPasses;

		StrMap<String> _shaderVariantPipelineMap;
	};

}