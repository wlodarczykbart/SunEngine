#pragma once

#include "Types.h"
#include "Scene.h"
#include "PipelineSettings.h"
#include "GraphicsPipeline.h"
#include "UniformBuffer.h"
#include "BaseShader.h"
#include "Material.h"
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

	enum RenderPassType
	{
		RPT_OUTPUT,
		RPT_GBUFFER,
		RPT_DEFERRED_RESOLVE,
		RPT_DEFERRED_COPY,
		RPT_SSR,
		RPT_SSR_BLUR,
		RPT_SSR_COPY,
		RPT_MSAA_RESOLVE,
	};

	struct RenderPassInfo
	{
		RenderTarget* pTarget;
		GraphicsPipeline* pPipeline;
		ShaderBindings* pBindings;
	};

	class SceneRenderer
	{
	public:	


		SceneRenderer();
		~SceneRenderer();

		bool Init();
		bool PrepareFrame(BaseTexture* pOutputTexture, bool updateTextures, CameraComponentData* pCamera = 0);
		bool RenderFrame(CommandBuffer* cmdBuffer, RenderTarget* pOpaqueTarget, const Map<RenderPassType, RenderPassInfo>& renderPasses);

		BaseTexture* GetShadowMapTexture() const { return _depthTarget.GetDepthTexture(); }
		void SetCascadeSplitLambda(float lambda) { _cascadeSplitLambda = lambda; }

		void RegisterShader(BaseShader* pShader) { _registeredShaders.insert(pShader); }
		bool BindEnvDataBuffer(CommandBuffer* cmdBuffer, BaseShader* pShader) const;

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
			AABB FrustumBox;
			UniquePtr<CameraComponentData> CameraData;
			uint CameraIndex;
		};

		struct ReflectionProbeData
		{
			ReflectionProbeData();

			bool NeedsUpdate;
			Vector<glm::vec3> ProbeCenters;
			uint CurrentUpdateProbe;
			uint CurrentUpdateFace;
			RenderTarget Target;
			Material EnvFaceCopyMaterial[6];
			UniformBufferGroup ObjectBufferGroup;
			UniformBufferGroup SkinnedBonesBufferGroup;
			LinkedList<RenderNodeData> RenderList;
			UniquePtr<CameraComponentData> CameraData;
			uint CameraIndex;
		};

		void ProcessRenderNode(RenderNode* pNode);
		void ProcessDepthRenderNode(RenderNode* pNode, DepthRenderData* pDepthData);
		void ProcessRenderList(CommandBuffer* cmdBuffer, LinkedList<RenderNodeData>& renderList, uint cameraUpdateIndex = 0, bool isDepth = false);
		bool GetPipeline(RenderNodeData& node, bool& sorted, bool isShadow = false);
		bool TryBindBuffer(CommandBuffer* cmdBuffer, BaseShader* pShader, UniformBufferData* buffer, IBindState* pBindState = 0) const;
		void RenderEnvironment(CommandBuffer* cmdBuffer);
		void RenderCommand(CommandBuffer* cmdBuffer, GraphicsPipeline* pPipeline, ShaderBindings* pBindings, uint vertexCount = 6, uint cameraUpdateIndex = 0);
		bool CreateDepthMaterial(Material* pMaterial, uint64 variantMask, Material* pEmptyMaterial) const;
		uint64 CalculateDepthVariantHash(Material* pMaterial, uint64 variantMask) const;
		void UpdateShadowCascades(Vector<CameraBufferData>& cameraBuffersToFill);
		bool ShouldRender(const RenderNode* pNode) const;
		uint64 GetVariantMask(const RenderNode* pNode) const;
		bool PerformSkinningCheck(const RenderNode* pNode);
		void UpdateEnvironmentProbes(Vector<CameraBufferData>& cameraBuffersToFill);
		void RenderEnvironmentProbes(CommandBuffer* cmdBuffer);

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
		float _cascadeSplitLambda;

		Vector<ShaderMat4> _skinnedBoneMatrixBlock;

		StrMap<GraphicsPipeline> _helperPipelines;

		RenderTarget _envTarget;
		ReflectionProbeData _envProbeData;
		AABB _shadowCasterAABB;

		HashSet<BaseShader*> _registeredShaders;
	};

}