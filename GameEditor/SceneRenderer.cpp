#include "SceneMgr.h"
#include "Camera.h"
#include "Light.h"
#include "RenderObject.h"
#include "Material.h"
#include "Shader.h"
#include "GraphicsPipeline.h"
#include "Mesh.h"
#include "CommandBuffer.h"
#include "RenderTarget.h"
#include "FilePathMgr.h"
#include "ShaderMgr.h"
#include "ResourceMgr.h"
#include "SceneRenderer.h"
#include "StringUtil.h"
#include "Environment.h"
#include "Animation.h"

#include <DirectXMath.h>

namespace SunEngine
{
	namespace HelperPipelines
	{
		const String SkyCopy = "SkyCopy";
	}

	SceneRenderer::SceneRenderer()
	{
		_bInit = false;
		_currentCamera = 0;
		_currentEnvironment = 0;
		_environmentBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
		_shadowMatrixBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	bool SceneRenderer::Init()
	{
		UniformBuffer::CreateInfo uboInfo = {};
		uboInfo.isShared = false;

		uboInfo.size = sizeof(EnvBufferData);
		if (!_environmentBuffer->Buffer.Create(uboInfo))
			return false;

		if (!_cameraGroup.Init(ShaderStrings::CameraBufferName, SBT_CAMERA, sizeof(CameraBufferData)))
			return false;

		if (!_objectBufferGroup.Init(ShaderStrings::ObjectBufferName, SBT_OBJECT, sizeof(ObjectBufferData)))
			return false;

		const uint MAX_SKINNED_BONES = EngineInfo::GetRenderer().MaxSkinnedBoneMatrices();
		if (!_skinnedBonesBufferGroup.Init(ShaderStrings::SkinnedBoneBufferName, SBT_BONES, sizeof(glm::mat4) * MAX_SKINNED_BONES))
			return false;
		_skinnedBoneMatrixBlock.resize(MAX_SKINNED_BONES);

		if (EngineInfo::GetRenderer().ShadowsEnabled())
		{
			_depthPasses.resize(EngineInfo::GetRenderer().MaxShadowCascadeSplits());
			uint depthSliceSize = EngineInfo::GetRenderer().CascadeShadowMapResolution();

			RenderTarget::CreateInfo depthTargetInfo = {};
			depthTargetInfo.hasDepthBuffer = true;
			depthTargetInfo.width = depthSliceSize * _depthPasses.size();
			depthTargetInfo.height = depthSliceSize;

			//depthTargetInfo.floatingPointColorBuffer = false;
			depthTargetInfo.numTargets = 0;

			if (!_depthTarget.Create(depthTargetInfo))
				return false;

			float vpWidth = (float)depthSliceSize / depthTargetInfo.width;
			float vpOffset = 0.0f;

			for (uint i = 0; i < _depthPasses.size(); i++)
			{
				_depthPasses[i] = UniquePtr<DepthRenderData>(new DepthRenderData());
				if (!_depthPasses[i]->ObjectBufferGroup.Init(ShaderStrings::ObjectBufferName, SBT_OBJECT, sizeof(ObjectBufferData)))
					return false;

				if (!_depthPasses[i]->SkinnedBonesBufferGroup.Init(ShaderStrings::SkinnedBoneBufferName, SBT_BONES, sizeof(glm::mat4) * MAX_SKINNED_BONES))
					return false;

				_depthPasses[i]->Viewport = { vpOffset, 0.0f, vpWidth, 1.0f };
				vpOffset += vpWidth;
			}

		}

		uboInfo.size = sizeof(glm::mat4) * EngineInfo::GetRenderer().MaxShadowCascadeSplits();
		if (!_shadowMatrixBuffer->Buffer.Create(uboInfo))
			return false;
		_shaderVariantPipelineMap[Shader::Depth] = DefaultPipelines::ShadowDepth;

		GraphicsPipeline::CreateInfo skyPipelineInfo = {};
		skyPipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_LESS_EQUAL;
		skyPipelineInfo.settings.rasterizer.frontFace = SE_FF_CLOCKWISE;

		skyPipelineInfo.pShader = ShaderMgr::Get().GetShader(DefaultShaders::Skybox)->GetDefault();
		if (!_helperPipelines[DefaultShaders::Skybox].Create(skyPipelineInfo))
			return false;

		skyPipelineInfo.pShader = ShaderMgr::Get().GetShader(DefaultShaders::SkyArHosek)->GetDefault();
		if (!_helperPipelines[DefaultShaders::SkyArHosek].Create(skyPipelineInfo))
			return false;

		GraphicsPipeline::CreateInfo cloudPipelineInfo = {};
		cloudPipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_LESS_EQUAL;
		cloudPipelineInfo.pShader = ShaderMgr::Get().GetShader(DefaultShaders::Clouds)->GetDefault();
		cloudPipelineInfo.settings.EnableAlphaBlend();
		if (!_helperPipelines[DefaultShaders::Clouds].Create(cloudPipelineInfo))
			return false;

		//Create sky target
		{
			RenderTarget::CreateInfo skyTargetInfo = {};
			skyTargetInfo.hasDepthBuffer = false;
			skyTargetInfo.floatingPointColorBuffer = true;
			skyTargetInfo.width = 512; //TODO read from engineinfo
			skyTargetInfo.height = 512;
			skyTargetInfo.numTargets = 1;
			if (!_skyTarget.Create(skyTargetInfo))
				return false;

			BaseShader* pSkyCopyShader = ShaderMgr::Get().GetShader(DefaultShaders::TextureCopy)->GetVariant(Shader::OneZ);
			ShaderBindings::CreateInfo skyBindingInfo = {};
			skyBindingInfo.pShader = pSkyCopyShader;
			skyBindingInfo.type = SBT_MATERIAL;

			if (!_skyBindings.Create(skyBindingInfo))
				return false;
			if (!_skyBindings.SetTexture(MaterialStrings::DiffuseMap, _skyTarget.GetColorTexture()))
				return false;
			if (!_skyBindings.SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE)))
				return false;

			GraphicsPipeline::CreateInfo skyCopyPipelineInfo = {};
			skyCopyPipelineInfo.pShader = pSkyCopyShader;
			skyCopyPipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_EQUAL;
			skyCopyPipelineInfo.settings.depthStencil.enableDepthWrite = false;
			if (!_helperPipelines[HelperPipelines::SkyCopy].Create(skyCopyPipelineInfo))
				return false;
		}

		_bInit = true;
		return true;
	}

	bool SceneRenderer::PrepareFrame(BaseTexture* pOutputTexture, bool updateTextures, CameraComponentData* pCamera)
	{
		if (!_bInit)
			return false;

		Scene* pScene = SceneMgr::Get().GetActiveScene();
		if (!pScene)
			return false;

		_currentCamera = pCamera;
		_currentEnvironment = 0;

		if (_currentCamera == 0)
		{
			auto& list = pScene->GetCameraList();
			_currentCamera = list.size() ? *list.begin() : 0;
		}

		//TODO: add support for point/spot lights
		//auto& lightList = pScene->GetLightList();
		//for (auto& light : lightList)
		//{
		//	const Light* pLight = light->C()->As<Light>();
		//	if (pLight->GetLightType() == LT_DIRECTIONAL)
		//		_currentSunlight = light;
		//}

		auto& environmentList = pScene->GetEnvironmentList();
		float deltaTime = 0.0f, elapsedTime = 0.0f;
		for (auto& env : environmentList)
		{
			_currentEnvironment = static_cast<const Environment*>(env->C());
			deltaTime = env->GetDeltaTime();
			elapsedTime = env->GetElapsedTime();
		}

		pScene->TraverseRenderNodes(
			[](const AABB& aabb, void* pAABBData) -> bool { return static_cast<CameraComponentData*>(pAABBData)->FrustumIntersects(aabb); }, _currentCamera, 
			[](RenderNode* pNode, void* pNodeData) -> void { static_cast<SceneRenderer*>(pNodeData)->ProcessRenderNode(pNode); }, this);

		//push current udpates to buffer
		_objectBufferGroup.Flush();
		_objectBufferGroup.Reset();

		_skinnedBonesBufferGroup.Flush();
		_skinnedBonesBufferGroup.Reset();

		for (uint i = 0; i < _depthPasses.size(); i++)
		{
			_depthPasses[i]->ObjectBufferGroup.Flush();
			_depthPasses[i]->ObjectBufferGroup.Reset();

			_depthPasses[i]->SkinnedBonesBufferGroup.Flush();
			_depthPasses[i]->SkinnedBonesBufferGroup.Reset();
		}

		if (!_currentCamera)
			return false;

		if (!_currentEnvironment)
			return false;

		CameraBufferData camData = {};
		glm::mat4 view = _currentCamera->GetView();
		glm::mat4 proj = _currentCamera->C()->As<Camera>()->GetProj();
		Shader::FillMatrices(view, proj, camData);
		glm::mat4 invView = *reinterpret_cast<glm::mat4*>(camData.InvViewMatrix.data);
		camData.Viewport.Set(0.0f, 0.0f, pOutputTexture->GetWidth(), pOutputTexture->GetHeight());

		uint camUpdateIndex = 0;
		_cameraGroup.Update(&camData, camUpdateIndex, &_cameraBuffer);

		glm::mat4 shadowMatrices[16];
		for (uint i = 0; i < _depthPasses.size(); i++)
		{
			float sceneRadius = 20.0f;
			glm::vec3 lightDir = _currentEnvironment->GetSunDirection();
			glm::vec3 lightPos = -2.0f * lightDir * sceneRadius;
			glm::vec3 targetPos = glm::vec3(invView[3]);
			targetPos = glm::vec3(0);
			glm::vec3 lightUp = glm::vec3(0, 1, 0);
			view = glm::lookAt(lightPos, targetPos, lightUp);

			glm::vec3 sphereCenterLS = view * glm::vec4(targetPos, 1.0f);
			float l = sphereCenterLS.x - sceneRadius;
			float r = sphereCenterLS.x + sceneRadius;
			float b = sphereCenterLS.y - sceneRadius;
			float t = sphereCenterLS.y + sceneRadius;
			float n = sphereCenterLS.z - sceneRadius;
			float f = sphereCenterLS.z + sceneRadius;
			proj = /*EngineInfo::GetRenderer().ProjectionCorrection() **/ glm::orthoLH_ZO(l, r, b, t, n, f);

			//glm::mat4 dxMtx = *(glm::mat4*)&DirectX::XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

			//bool eq = true;
			//for (int ii = 0; ii < 4; ii++)
			//{
			//	for (int jj = 0; jj < 4; jj++) {
			//		if (fabsf(proj[ii][jj] - dxMtx[ii][jj]) > 0.0001)
			//		{
			//			eq = false;
			//		}
			//	}
			//}

			//lightPos = glm::vec3(-27.4350910, -35.4456558, 28.6385860);
			//lightPos = lightDir* sceneRadius;
			//proj = glm::perspective(glm::radians(45.0f), 1.0f, 1.0f, 96.0f);
			//view = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
			
			Shader::FillMatrices(view, proj, camData);
			camData.Viewport.Set(_depthPasses[i]->Viewport.x, _depthPasses[i]->Viewport.y, _depthPasses[i]->Viewport.width, _depthPasses[i]->Viewport.height);
			_cameraGroup.Update(&camData, camUpdateIndex, &_cameraBuffer);
			glm::mat4 viewProj = proj * view;
			_shadowMatrixBuffer->Buffer.Update(&viewProj, i * sizeof(glm::mat4), sizeof(glm::mat4));
		}

		_cameraGroup.Flush();
		_cameraGroup.Reset();

		Environment::FogSettings fog;
		_currentEnvironment->GetFogSettings(fog);

		EnvBufferData envData = {};
		glm::vec4 sunDir = glm::vec4(glm::normalize(_currentEnvironment->GetSunDirection()), 0.0);
		glm::vec4 sunDirView = _currentCamera->GetView() * sunDir;
		envData.SunColor.Set(1, 1, 1, 1);
		envData.SunDirection.Set(&sunDir.x);
		envData.SunViewDirection.Set(&sunDirView);
		envData.TimeData.Set(deltaTime, elapsedTime, 0, 0);
		envData.FogColor.Set(fog.color.x, fog.color.y, fog.color.z, 0.0f);
		envData.FogControls.Set(fog.enabled ? 1.0f : 0.0f, fog.sampleSky ? 1.0f : 0.0f, fog.density, 0.0f);
		if (!_environmentBuffer->Buffer.Update(&envData))
			return false;

		for (auto& pipeline : _helperPipelines)
			_currentShaders.insert(pipeline.second.GetShader());

		//plug in deferred shader so it gets camera/light buffers
		if (EngineInfo::GetRenderer().RenderMode() == EngineInfo::Renderer::Deferred)
		{
			_currentShaders.insert(ShaderMgr::Get().GetShader(DefaultShaders::Deferred)->GetDefault());
			_currentShaders.insert(ShaderMgr::Get().GetShader(DefaultShaders::ScreenSpaceReflection)->GetDefault());
		}

		for (auto iter = _currentShaders.begin(); iter != _currentShaders.end(); ++iter)
		{
			BaseShader* pShader = (*iter);

			if (pShader->ContainsBuffer(ShaderStrings::CameraBufferName) && _cameraBuffer->ShaderBindings.find(pShader) == _cameraBuffer->ShaderBindings.end())
			{
				ShaderBindings::CreateInfo bindInfo = {};
				bindInfo.pShader = pShader;
				bindInfo.type = SBT_CAMERA;
				if (!_cameraBuffer->ShaderBindings[pShader].Create(bindInfo))
					return false;
				if (!_cameraBuffer->ShaderBindings[pShader].SetUniformBuffer(ShaderStrings::CameraBufferName, &_cameraBuffer->Buffer))
					return false;
			}

			if (pShader->ContainsBuffer(ShaderStrings::EnvBufferName) && (_environmentBuffer->ShaderBindings.find(pShader) == _environmentBuffer->ShaderBindings.end()))
			{
				ShaderBindings::CreateInfo bindInfo = {};
				bindInfo.pShader = pShader;
				bindInfo.type = SBT_ENVIRONMENT;
				auto& binding = _environmentBuffer->ShaderBindings[pShader];
				if (!binding.Create(bindInfo))
					return false;
				if (!binding.SetUniformBuffer(ShaderStrings::EnvBufferName, &_environmentBuffer->Buffer))
					return false;

				bool skyTextureSet = binding.SetTexture(ShaderStrings::SkyTextureName, _skyTarget.GetColorTexture());
				bool skySamplerSet = binding.SetSampler(ShaderStrings::SkySamplerName, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE));
			}

			if (pShader->ContainsBuffer(ShaderStrings::ShadowBufferName) && _shadowMatrixBuffer->ShaderBindings.find(pShader) == _shadowMatrixBuffer->ShaderBindings.end())
			{
				ShaderBindings::CreateInfo bindInfo = {};
				bindInfo.pShader = pShader;
				bindInfo.type = SBT_SHADOW;
				auto& binding = _shadowMatrixBuffer->ShaderBindings[pShader];
				if (!binding.Create(bindInfo))
					return false;
				if (!binding.SetUniformBuffer(ShaderStrings::ShadowBufferName, &_shadowMatrixBuffer->Buffer))
					return false;
				bool depthTextureSet = binding.SetTexture(ShaderStrings::ShadowTextureName, EngineInfo::GetRenderer().ShadowsEnabled() ? _depthTarget.GetDepthTexture() : ResourceMgr::Get().GetTexture2D(DefaultResource::Texture::White)->GetGPUObject());
				bool depthSamplerSet = binding.SetSampler(ShaderStrings::ShadowSamplerName, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_BORDER, SE_BC_WHITE));
			}
		}

		return true;
	}

	bool SceneRenderer::RenderFrame(CommandBuffer* cmdBuffer, RenderTarget* pOpaqueTarget, RenderTargetPassInfo* pOutputInfo, DeferredRenderTargetPassInfo* pDeferredInfo)
	{
		if (!_currentCamera)
			return false;

		if (!_currentEnvironment)
			return false;

		BaseShader* pShader = 0;

		_depthTarget.Bind(cmdBuffer);
		for (uint i = 0; i < _depthPasses.size(); i++)
		{
			ProcessRenderList(cmdBuffer, _depthPasses[i]->RenderList, i+1);
		}
		_depthTarget.Unbind(cmdBuffer);

		uint mainCamUpdateIndex = 0;

		_skyTarget.Bind(cmdBuffer);
		RenderSky(cmdBuffer);
		_skyTarget.Unbind(cmdBuffer);

		if (pDeferredInfo)
		{
			pDeferredInfo->pTarget->Bind(cmdBuffer);
			ProcessRenderList(cmdBuffer, _gbufferRenderList, mainCamUpdateIndex);
			pDeferredInfo->pTarget->Unbind(cmdBuffer);

			//Draw deferred objects to the deferred resolve render target
			pDeferredInfo->pDeferredResolveTarget->Bind(cmdBuffer);

			pShader = pDeferredInfo->pPipeline->GetShader();
			pShader->Bind(cmdBuffer);
			pDeferredInfo->pPipeline->Bind(cmdBuffer);
			pDeferredInfo->pBindings->Bind(cmdBuffer);

			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer);
			TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
			TryBindBuffer(cmdBuffer, pShader, _shadowMatrixBuffer.get());

			//draw 2 triangles that are generated by screen vertex shader
			cmdBuffer->Draw(6, 1, 0, 0);

			pDeferredInfo->pBindings->Unbind(cmdBuffer);
			pDeferredInfo->pPipeline->Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);

			pDeferredInfo->pDeferredResolveTarget->Unbind(cmdBuffer);

			pOpaqueTarget->Bind(cmdBuffer);

			//copy deferred resolve to opaque render target
			pShader = pDeferredInfo->pDeferredCopyPipeline->GetShader();
			pShader->Bind(cmdBuffer);
			pDeferredInfo->pDeferredCopyPipeline->Bind(cmdBuffer);
			pDeferredInfo->pDeferredCopyBindings->Bind(cmdBuffer);

			//draw 2 triangles that are generated by screen vertex shader
			cmdBuffer->Draw(6, 1, 0, 0);

			pDeferredInfo->pDeferredCopyBindings->Unbind(cmdBuffer);
			pDeferredInfo->pDeferredCopyPipeline->Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);

			////apply screen space reflections
			if (pDeferredInfo->pSSRPipeline)
			{
				pShader = pDeferredInfo->pSSRPipeline->GetShader();
				pShader->Bind(cmdBuffer);
				pDeferredInfo->pSSRPipeline->Bind(cmdBuffer);
				pDeferredInfo->pSSRBindings->Bind(cmdBuffer);

				TryBindBuffer(cmdBuffer, pShader, _cameraBuffer);

				//draw 2 triangles that are generated by screen vertex shader
				cmdBuffer->Draw(6, 1, 0, 0);

				pDeferredInfo->pSSRBindings->Unbind(cmdBuffer);
				pDeferredInfo->pSSRPipeline->Unbind(cmdBuffer);
				pShader->Unbind(cmdBuffer);
			}
		}
		else
		{
			pOpaqueTarget->Bind(cmdBuffer);
		}

		ProcessRenderList(cmdBuffer, _opaqueRenderList, mainCamUpdateIndex);

		RenderEnvironment(cmdBuffer);

		pOpaqueTarget->Unbind(cmdBuffer);

		pOutputInfo->pTarget->Bind(cmdBuffer);
		if(true)
		{
			//Copy opaque to output
			pShader = pOutputInfo->pPipeline->GetShader();
			pShader->Bind(cmdBuffer);
			pOutputInfo->pPipeline->Bind(cmdBuffer);
			pOutputInfo->pBindings->Bind(cmdBuffer);

			//draw 2 triangles that are generated by screen vertex shader
			cmdBuffer->Draw(6, 1, 0, 0);

			pOutputInfo->pBindings->Unbind(cmdBuffer);
			pOutputInfo->pPipeline->Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);
		}

		_sortedRenderList.sort([](const RenderNodeData& left, const RenderNodeData& right) -> bool { return left.SortingDistance > right.SortingDistance; });
		ProcessRenderList(cmdBuffer, _sortedRenderList, mainCamUpdateIndex);

		pOutputInfo->pTarget->Unbind(cmdBuffer);

		_currentShaders.clear();
		return true;
	}

	void SceneRenderer::ProcessRenderNode(RenderNode* pNode)
	{
		Material* pMaterial = pNode->GetMaterial();
		bool isValid = pMaterial && pMaterial->GetShader() && pNode->GetMesh();
		

		if (!isValid)
			return;

		SceneNode* pSceneNode = pNode->GetNode();
		//if (!StrContains(pSceneNode->GetName(), "Frustum"))
		{
			if (!_currentCamera->FrustumIntersects(pNode->GetWorldAABB()))
				return;
		}

		Component* pSMComponent = pSceneNode->GetComponentOfType(COMPONENT_SKINNED_MESH);
		SkinnedMesh* pSkinnedMesh = 0;
		if (pSMComponent)
		{
			pSkinnedMesh = pSMComponent->As<SkinnedMesh>();
			SkinnedMeshComponentData* pSkinnedData = pSceneNode->GetComponentData<SkinnedMeshComponentData>(pSMComponent);
			AnimatorComponentData* pAnimatorData = pSkinnedData->GetAnimatorData();

			if (pAnimatorData->GetPlaying())
			{
				auto& boneData = pAnimatorData->GetBoneData();
				for (uint i = 0; i < boneData.size(); i++)
				{
					String name = boneData[i]->GetNode()->GetName();
					glm::mat4 boneMtx = boneData[i]->GetNode()->GetWorld();
					boneMtx = glm::inverse(pNode->GetWorld()) * boneMtx * boneData[i]->C()->As<const AnimatedBone>()->GetSkinMatrix(pSkinnedMesh->GetSkinIndex());	//TODO check for correct order...
					_skinnedBoneMatrixBlock[i].Set(&boneMtx);
				}
			}
			else
			{
				static glm::mat4 mtxIden(1.0f);
				for (uint i = 0; i < pAnimatorData->GetBoneCount(); i++)
					_skinnedBoneMatrixBlock[i].Set(&mtxIden);
			}
		}

		bool sorted = false;
		RenderNodeData data = {};
		data.SceneNode = pSceneNode;
		data.RenderNode = pNode;
		GetPipeline(data, sorted);

		BaseShader* pShader = pMaterial->GetShaderVariant();

		ObjectBufferData objBuffer = {};
		glm::mat4 itp = glm::transpose(glm::inverse(pNode->GetWorld()));
		objBuffer.WorldMatrix.Set(&pNode->GetWorld());
		objBuffer.InverseTransposeMatrix.Set(&itp);

		_objectBufferGroup.Update(&objBuffer, data.ObjectBufferIndex, &data.ObjectBindings, pShader);

		if (pSkinnedMesh)
			_skinnedBonesBufferGroup.Update(_skinnedBoneMatrixBlock.data(), data.SkinnedBoneBufferIndex, &data.SkinnedBoneBindings, pShader);

		_currentShaders.insert(pShader);

		if (!sorted)
		{
			if (pMaterial->GetVariant() == Shader::Default)
				_opaqueRenderList.push_back(data);
			else if (pMaterial->GetVariant() == Shader::GBuffer)
				_gbufferRenderList.push_back(data);
		}
		else
		{
			glm::vec3 vDelta = pNode->GetWorldAABB().GetCenter() - _currentCamera->GetPosition();
			data.SortingDistance = glm::dot(vDelta, vDelta);
			_sortedRenderList.push_back(data);
		}

		if (EngineInfo::GetRenderer().ShadowsEnabled())
		{
			BaseShader* pDepthShader = pMaterial->GetShader()->GetVariant(Shader::Depth);
			if (pDepthShader)
			{
				usize depthHash = data.RenderNode->GetMaterial()->GetDepthVariantHash();
				auto pDepthMaterial = _depthMaterials[depthHash].get();
				if (pDepthMaterial == 0)
				{
					pDepthMaterial = new Material();
					pMaterial->CreateDepthMaterial(pDepthMaterial);
					_depthMaterials[depthHash] = UniquePtr<Material>(pDepthMaterial);
				}

				for (uint i = 0; i < _depthPasses.size(); i++)
				{
					RenderNodeData depthData = data;
					depthData.MaterialOverride = pDepthMaterial;
					GetPipeline(depthData, sorted);
					_depthPasses[i]->ObjectBufferGroup.Update(&objBuffer, depthData.ObjectBufferIndex, &depthData.ObjectBindings, pDepthShader);

					if (pSkinnedMesh)
						_depthPasses[i]->SkinnedBonesBufferGroup.Update(_skinnedBoneMatrixBlock.data(), depthData.SkinnedBoneBufferIndex, &depthData.SkinnedBoneBindings, pDepthShader);

					_depthPasses[i]->RenderList.push_back(depthData);
				}

				_currentShaders.insert(pDepthShader);
			}
		}
	}

	void SceneRenderer::ProcessRenderList(CommandBuffer* cmdBuffer, LinkedList<RenderNodeData>& renderList, uint cameraUpdateIndex)
	{
		IShaderBindingsBindState objectBindData = {};
		objectBindData.DynamicIndices[0].first = ShaderStrings::ObjectBufferName;

		IShaderBindingsBindState cameraBindData = {};
		cameraBindData.DynamicIndices[0] = { ShaderStrings::CameraBufferName, cameraUpdateIndex };

		IShaderBindingsBindState skinnedBoneBindData = {};
		skinnedBoneBindData.DynamicIndices[0].first = ShaderStrings::SkinnedBoneBufferName;

		for(auto& renderData : renderList)
		{
			Material* pMaterial = renderData.MaterialOverride ? renderData.MaterialOverride : renderData.RenderNode->GetMaterial();

			BaseShader* pShader = pMaterial->GetShaderVariant();
			GraphicsPipeline& pipeline = *renderData.Pipeline;

			//pipeline.GetShader()->getgpu
			pShader->Bind(cmdBuffer);
			pipeline.Bind(cmdBuffer);
			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer, &cameraBindData);
			TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
			TryBindBuffer(cmdBuffer, pShader, _shadowMatrixBuffer.get());

			renderData.RenderNode->GetMesh()->GetGPUObject()->Bind(cmdBuffer);
			pMaterial->GetGPUObject()->Bind(cmdBuffer);

			objectBindData.DynamicIndices[0].second = renderData.ObjectBufferIndex;
			renderData.ObjectBindings->ShaderBindings.at(pShader).Bind(cmdBuffer, &objectBindData);

			if (renderData.SkinnedBoneBindings)
			{
				skinnedBoneBindData.DynamicIndices[0].second = renderData.SkinnedBoneBufferIndex;
				renderData.SkinnedBoneBindings->ShaderBindings.at(pShader).Bind(cmdBuffer, &skinnedBoneBindData);
			}

			cmdBuffer->DrawIndexed(
				renderData.RenderNode->GetIndexCount(),
				renderData.RenderNode->GetInstanceCount(),
				renderData.RenderNode->GetFirstIndex(),
				renderData.RenderNode->GetVertexOffset(),
				0);

			pipeline.Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);
		}
		renderList.clear();
	}

	bool SceneRenderer::GetPipeline(RenderNodeData& data, bool& sorted)
	{
		sorted = false;
		PipelineSettings settings = {};
		data.RenderNode->BuildPipelineSettings(settings);

		Material* pMaterial = data.MaterialOverride ? data.MaterialOverride : data.RenderNode->GetMaterial();
		BaseShader* pShader = pMaterial->GetShaderVariant();

		auto variantPipeline = _shaderVariantPipelineMap.find(pMaterial->GetVariant());
		if (variantPipeline != _shaderVariantPipelineMap.end())
			ShaderMgr::Get().BuildPipelineSettings((*variantPipeline).second, settings);

		glm::vec4 diffuseColor;
		if (pMaterial->GetMaterialVar(MaterialStrings::DiffuseColor, diffuseColor) && diffuseColor.a < 1.0f)
		{
			settings.EnableAlphaBlend();
			sorted = true;
		}

		for (uint i = 0; i < _graphicsPipelines.size(); i++)
		{
			if (_graphicsPipelines[i]->GetShader() == pShader && settings == _graphicsPipelines[i]->GetSettings())
			{
				data.Pipeline = _graphicsPipelines[i].get();
				return true;
			}
		}

		GraphicsPipeline* pipeline = new GraphicsPipeline();

		GraphicsPipeline::CreateInfo info = {};
		info.pShader = pShader;
		info.settings = settings;

		if (!pipeline->Create(info))
			return false;

		_graphicsPipelines.push_back(UniquePtr<GraphicsPipeline>(pipeline));
		data.Pipeline = pipeline;
		return true;
	}

	void SceneRenderer::TryBindBuffer(CommandBuffer* cmdBuffer, BaseShader* pShader, UniformBufferData* buffer, IBindState* pBindState) const
	{
		auto found = buffer->ShaderBindings.find(pShader);
		if(found != buffer->ShaderBindings.end())
			(*found).second.Bind(cmdBuffer, pBindState);
	}

	void SceneRenderer::RenderSky(CommandBuffer* cmdBuffer)
	{
		SkyModel* pSkyModel = _currentEnvironment->GetActiveSkyModel();
		Material* pMaterial = pSkyModel->GetMaterial();
		BaseShader* pShader = pMaterial->GetShaderVariant();
		auto& pipeline = _helperPipelines.at(pMaterial->GetShader()->GetName());
		SkyModel::MeshType meshType = pSkyModel->GetMeshType();

		Mesh* pMesh = 0;
		switch (meshType)
		{
		case SunEngine::SkyModel::MT_CUBE:
			pMesh = ResourceMgr::Get().GetMesh(DefaultResource::Mesh::Cube);
			break;
		case SunEngine::SkyModel::MT_SPHERE:
			pMesh = ResourceMgr::Get().GetMesh(DefaultResource::Mesh::Sphere);
			break;
		case SunEngine::SkyModel::MT_QUAD:
			pMesh = ResourceMgr::Get().GetMesh(DefaultResource::Mesh::Quad);
			break;
		default:
			break;
		}

		pShader->Bind(cmdBuffer);
		pipeline.Bind(cmdBuffer);
		pMaterial->GetGPUObject()->Bind(cmdBuffer);
		TryBindBuffer(cmdBuffer, pShader, _cameraBuffer);
		TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
		if (pMesh)
		{
			pMesh->GetGPUObject()->Bind(cmdBuffer);
			cmdBuffer->DrawIndexed(pMesh->GetIndexCount(), 1, 0, 0, 0);
		}
		else
		{
			//screen quad default, is generated in shader
			cmdBuffer->Draw(6, 1, 0, 0);
		}
		pMaterial->GetGPUObject()->Unbind(cmdBuffer);
		pipeline.Unbind(cmdBuffer);
		pShader->Unbind(cmdBuffer);
	}

	void SceneRenderer::RenderEnvironment(CommandBuffer* cmdBuffer)
	{
		BaseShader* pShader = 0;
		ShaderBindings* pBindings = 0;

		//Copy the texture containing the sky background to main framebuffer
		pBindings = &_skyBindings;
		if (pBindings)
		{
			auto& pipeline = _helperPipelines.at(HelperPipelines::SkyCopy);
			pShader = pipeline.GetShader();
			pShader->Bind(cmdBuffer);
			pipeline.Bind(cmdBuffer);
			pBindings->Bind(cmdBuffer);
			cmdBuffer->Draw(6, 1, 0, 0);
			pBindings->Unbind(cmdBuffer);
			pipeline.Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);
		}

		//Draw clouds over sky
		pBindings = _currentEnvironment->GetCloudBindings();
		if (pBindings)
		{
			auto& pipeline = _helperPipelines.at(DefaultShaders::Clouds);
			pShader = pipeline.GetShader();
			pShader->Bind(cmdBuffer);
			pipeline.Bind(cmdBuffer);
			pBindings->Bind(cmdBuffer);
			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer);
			TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
			cmdBuffer->Draw(6, 1, 0, 0);
			pBindings->Unbind(cmdBuffer);
			pipeline.Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);
		}
	}


	SceneRenderer::UniformBufferGroup::UniformBufferGroup()
	{
		_current = 0;
		_blockSize = 0;
		_maxUpdates = 0;
		_bindType = {};
	}

	bool SceneRenderer::UniformBufferGroup::Init(const String& bufferName, ShaderBindingType bindType, uint blockSize)
	{
		_name = bufferName;
		_bindType = bindType;
		_blockSize = blockSize;

		UniformBuffer::CreateInfo uboInfo = {};

		_current = new UniformBufferData();
		uboInfo.isShared = true;
		uboInfo.size = blockSize;
		if (!_current->Buffer.Create(uboInfo))
			return false;
		_current->ArrayIndex = 0;
		_buffers.push_back(UniquePtr<UniformBufferData>(_current));
		_maxUpdates = _current->Buffer.GetMaxSharedUpdates();
		_data.SetSize(_maxUpdates * blockSize);

		return true;
	}

	void SceneRenderer::UniformBufferGroup::Flush()
	{
		_current->Buffer.UpdateShared(_data.GetData(), _current->UpdateIndex);
	}

	void SceneRenderer::UniformBufferGroup::Reset()
	{
		_current = _buffers[0].get();
		_current->UpdateIndex = 0;
	}

	void SceneRenderer::UniformBufferGroup::Update(const void* dataBlock, uint& updatedIndex, UniformBufferData** ppUpdatedBuffer, BaseShader* pShader)
	{
		if (_current->UpdateIndex == _maxUpdates)
		{
			//push current udpates to buffer
			_current->Buffer.UpdateShared(_data.GetData(), _maxUpdates);

			//put this is some generic method if adding more of these types of sharable ubos?
			if (_current->ArrayIndex == _buffers.size() - 1)
			{
				_current = new UniformBufferData();
				UniformBuffer::CreateInfo buffInfo = {};
				buffInfo.isShared = true;
				buffInfo.size = _blockSize;
				if (!_current->Buffer.Create(buffInfo))
					return;

				_buffers.push_back(UniquePtr<UniformBufferData>(_current));
				_current->ArrayIndex = _buffers.size() - 1;
			}
			else
			{
				_current = _buffers[_current->ArrayIndex + 1].get();
			}

			_current->UpdateIndex = 0;
		}


		_data.SetData(dataBlock, _blockSize, _current->UpdateIndex * _blockSize);

		updatedIndex = _current->UpdateIndex;
		if(ppUpdatedBuffer) *ppUpdatedBuffer = _current;
		_current->UpdateIndex++;

		if (pShader && _current->ShaderBindings.find(pShader) == _current->ShaderBindings.end())
		{
			ShaderBindings::CreateInfo bindingInfo = {};
			bindingInfo.pShader = pShader;
			bindingInfo.type = _bindType;
			_current->ShaderBindings[pShader].Create(bindingInfo);
			_current->ShaderBindings[pShader].SetUniformBuffer(_name, &_current->Buffer);
		}
	}
}