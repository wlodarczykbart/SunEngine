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
	class Environment;

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
		bool PrepareFrame(BaseTexture* pOutputTexture, bool updateTextures, CameraComponentData* pCamera = 0);
		bool RenderFrame(CommandBuffer* cmdBuffer, RenderTarget* pOpaqueTarget, RenderTargetPassInfo* pOutputInfo, DeferredRenderTargetPassInfo* pDeferredInfo, RenderTargetPassInfo* pMSAAResolveInfo);

		BaseTexture* GetShadowMapTexture() const { return _depthTarget.GetDepthTexture(); }

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
			const RenderNode* RenderNode;
			GraphicsPipeline* Pipeline;
			UniformBufferData* ObjectBindings;
			uint ObjectBufferIndex;
			Material* MaterialOverride;

			UniformBufferData* SkinnedBoneBindings;
			uint SkinnedBoneBufferIndex;

			uint64 BaseVariantMask;
			uint64 DepthHash;

			float SortingDistance;
		};

		struct DepthRenderData
		{
			DepthRenderData();

			UniformBufferGroup ObjectBufferGroup;
			UniformBufferGroup SkinnedBonesBufferGroup;
			LinkedList<RenderNodeData> RenderList;
			Viewport Viewport;
			AABB FrustumBox;
			UniquePtr<CameraComponentData> CameraData;
		};

		void ProcessRenderNode(RenderNode* pNode);
		void ProcessDepthRenderNode(RenderNode* pNode, DepthRenderData* pDepthData);
		void ProcessRenderList(CommandBuffer* cmdBuffer, LinkedList<RenderNodeData>& renderList, uint cameraUpdateIndex = 0, bool isDepth = false);
		bool GetPipeline(RenderNodeData& node, bool& sorted, bool isDepth = false, bool isShadow = false);
		bool TryBindBuffer(CommandBuffer* cmdBuffer, BaseShader* pShader, UniformBufferData* buffer, IBindState* pBindState = 0) const;
		void RenderSky(CommandBuffer* cmdBuffer);
		void RenderEnvironment(CommandBuffer* cmdBuffer);
		void RenderCommand(CommandBuffer* cmdBuffer, GraphicsPipeline* pPipeline, ShaderBindings* pBindings, uint vertexCount = 6);
		bool CreateDepthMaterial(Material* pMaterial, uint64 variantMask, Material* pEmptyMaterial) const;
		uint64 CalculateDepthVariantHash(Material* pMaterial, uint64 variantMask) const;
		void UpdateShadowCascades(Vector<CameraBufferData>& cameraBuffersToFill);
		bool ShouldRender(const RenderNode* pNode) const;
		uint64 GetVariantMask(const RenderNode* pNode) const;

		bool _bInit;
		UniquePtr<UniformBufferData> _cameraBuffer;
		UniquePtr<UniformBufferData> _environmentBuffer;
		UniquePtr<UniformBufferData> _shadowBuffer;
		Vector<UniquePtr<GraphicsPipeline>> _graphicsPipelines;
		UniformBufferGroup _objectBufferGroup;
		UniformBufferGroup _skinnedBonesBufferGroup;
		CameraComponentData* _currentCamera;
		const Environment* _currentEnvironment;
		HashSet<BaseShader*> _currentShaders;
		LinkedList<RenderNodeData> _gbufferRenderList;
		LinkedList<RenderNodeData> _opaqueRenderList;
		LinkedList<RenderNodeData> _sortedRenderList;

		Map<usize, UniquePtr<Material>> _depthMaterials;
		RenderTarget _depthTarget;
		Vector<UniquePtr<DepthRenderData>> _depthPasses;

		Vector<ShaderMat4> _skinnedBoneMatrixBlock;

		StrMap<GraphicsPipeline> _helperPipelines;

		RenderTarget _skyTarget;
		ShaderBindings _skyBindings;
	};

}