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

		class UniformBufferGroup
		{
		public:
			UniformBufferGroup();
			bool Init(const String& bufferName, ShaderBindingType bindType, uint blockSize);
			void Flush();
			void Reset();
			void Update(const void* dataBlock, uint& updatedIndex, UniformBufferData** ppUpdatedBuffer = 0, BaseShader* pShader = 0);

		private:
			String _name;
			ShaderBindingType _bindType;
			uint _blockSize;
			uint _maxUpdates;

			Vector<UniquePtr<UniformBufferData>> _buffers;
			MemBuffer _data;
			UniformBufferData* _current;
		};

		struct RenderNodeData
		{
			SceneNode* SceneNode;
			const RenderNode* RenderNode;
			GraphicsPipeline* Pipeline;
			UniformBufferData* ObjectBindings;
			uint ObjectBufferIndex;
			Material* MaterialOverride;

			UniformBufferData* SkinnedBoneBindings;
			uint SkinnedBoneBufferIndex;
		};

		struct DepthRenderData
		{
			UniformBufferGroup ObjectBufferGroup;
			UniformBufferGroup SkinnedBonesBufferGroup;
			Queue<RenderNodeData> RenderQueue;
			Viewport Viewport;
		};

		static void TraverseFunc(SceneNode* data, void* pUserData);
		void ProcessNode(SceneNode* pNode);
		void ProcessRenderQueue(CommandBuffer* cmdBuffer, Queue<RenderNodeData>& queue, uint cameraUpdateIndex);
		bool GetPipeline(RenderNodeData& node);
		void TryBindBuffer(CommandBuffer* cmdBuffer, BaseShader* pShader, UniformBufferData* buffer, IBindState* pBindState = 0) const;

		bool _bInit;
		UniformBufferGroup _cameraGroup;
		UniformBufferData* _cameraBuffer;
		UniquePtr<UniformBufferData> _lightBuffer;
		UniquePtr<UniformBufferData> _shadowMatrixBuffer;
		Vector<UniquePtr<GraphicsPipeline>> _graphicsPipelines;
		UniformBufferGroup _objectBufferGroup;
		UniformBufferGroup _skinnedBonesBufferGroup;
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
		Vector<ShaderMat4> _skinnedBoneMatrixBlock;
	};

}