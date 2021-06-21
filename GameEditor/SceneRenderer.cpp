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
#include "StringUtil.h"
#include "Environment.h"
#include "CascadedShadowMap.h"
#include "GraphicsWindow.h"
#include "Animation.h"

#include "SceneRenderer.h"

namespace SunEngine
{
	namespace HelperPipelines
	{
		const String EnvCopy = "EnvCopy";
	}

	SceneRenderer::SceneRenderer()
	{
		_bInit = false;
		_currentCamera = 0;
		_currentEnvironment = 0;
		_cameraBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
		_environmentBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
		_shadowBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
		_cascadeSplitLambda = 0.0f;
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	bool SceneRenderer::Init()
	{
		UniformBuffer::CreateInfo uboInfo = {};
		uboInfo.isShared = true;

		uboInfo.size = sizeof(EnvBufferData);
		if (!_environmentBuffer->Buffer.Create(uboInfo))
			return false;

		uboInfo.size = sizeof(ShadowBufferData);
		if (!_shadowBuffer->Buffer.Create(uboInfo))
			return false;

		uboInfo.size = sizeof(CameraBufferData);
		if (!_cameraBuffer->Buffer.Create(uboInfo))
			return false;

		if (!_objectBufferGroup.Init(ShaderStrings::ObjectBufferName, SBT_OBJECT, sizeof(ObjectBufferData)))
			return false;

		const uint skinnedBoneCount = EngineInfo::GetRenderer().SkinnedBoneMatrices();
		if (!_skinnedBonesBufferGroup.Init(ShaderStrings::SkinnedBoneBufferName, SBT_BONES, sizeof(glm::mat4) * skinnedBoneCount))
			return false;
		_skinnedBoneMatrixBlock.resize(skinnedBoneCount);

		if (EngineInfo::GetRenderer().ShadowsEnabled())
		{
			_depthPasses.resize(EngineInfo::GetRenderer().CascadeShadowMapSplits());
			uint depthSliceSize = EngineInfo::GetRenderer().CascadeShadowMapResolution();

			RenderTarget::CreateInfo depthTargetInfo = {};
			depthTargetInfo.hasDepthBuffer = true;
			depthTargetInfo.width = depthSliceSize;
			depthTargetInfo.height = depthSliceSize;
			depthTargetInfo.numLayers =_depthPasses.size();

			//depthTargetInfo.floatingPointColorBuffer = false;
			depthTargetInfo.numTargets = 0;

			if (!_depthTarget.Create(depthTargetInfo))
				return false;

			for (uint i = 0; i < _depthPasses.size(); i++)
			{
				_depthPasses[i] = UniquePtr<DepthRenderData>(new DepthRenderData());
				if (!_depthPasses[i]->ObjectBufferGroup.Init(ShaderStrings::ObjectBufferName, SBT_OBJECT, sizeof(ObjectBufferData)))
					return false;

				if (!_depthPasses[i]->SkinnedBonesBufferGroup.Init(ShaderStrings::SkinnedBoneBufferName, SBT_BONES, sizeof(glm::mat4) * skinnedBoneCount))
					return false;
			}

		}

		GraphicsPipeline::CreateInfo skyPipelineInfo = {};
		skyPipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_LESS_EQUAL;
		//skyPipelineInfo.settings.rasterizer.frontFace = SE_FF_CLOCKWISE;

		skyPipelineInfo.pShader = ShaderMgr::Get().GetShader(DefaultShaders::Skybox)->GetBaseVariant(ShaderVariant::ONE_Z);
		if (!_helperPipelines[DefaultShaders::Skybox].Create(skyPipelineInfo))
			return false;

		skyPipelineInfo.pShader = ShaderMgr::Get().GetShader(DefaultShaders::SkyArHosek)->GetBase();
		if (!_helperPipelines[DefaultShaders::SkyArHosek].Create(skyPipelineInfo))
			return false;

		GraphicsPipeline::CreateInfo cloudPipelineInfo = {};
		cloudPipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_LESS_EQUAL;
		cloudPipelineInfo.pShader = ShaderMgr::Get().GetShader(DefaultShaders::Clouds)->GetBase();
		cloudPipelineInfo.settings.EnableAlphaBlend();
		if (!_helperPipelines[DefaultShaders::Clouds].Create(cloudPipelineInfo))
			return false;

		//Create env target
		{
			RenderTarget::CreateInfo envTargetInfo = {};
			envTargetInfo.hasDepthBuffer = false;
			envTargetInfo.floatingPointColorBuffer = true;
			envTargetInfo.width = 512; //TODO read from engineinfo
			envTargetInfo.height = 512;
			envTargetInfo.numTargets = 1;
			envTargetInfo.cubemap = true;
			if (!_envTarget.Create(envTargetInfo))
				return false;

			BaseShader* pEnvCopyShader = ShaderMgr::Get().GetShader(DefaultShaders::EnvTextureCopy)->GetBaseVariant(ShaderVariant::ONE_Z);
			GraphicsPipeline::CreateInfo envCopyPipelineInfo = {};
			envCopyPipelineInfo.pShader = pEnvCopyShader;
			envCopyPipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_EQUAL;
			envCopyPipelineInfo.settings.depthStencil.enableDepthWrite = false;
			if (!_helperPipelines[HelperPipelines::EnvCopy].Create(envCopyPipelineInfo))
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

		if (!_currentCamera)
			return false;

		if (!_currentEnvironment)
			return false;

		Vector<CameraBufferData> cameraDataList;

		CameraBufferData camData = {};
		glm::mat4 view = _currentCamera->GetView();
		glm::mat4 proj = _currentCamera->C()->As<Camera>()->GetProj();
		//proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 10.0f);
		Shader::FillMatrices(view, proj, camData);
		camData.CameraData.row0.Set(0.0f, 0.0f, (float)pOutputTexture->GetWidth(), (float)pOutputTexture->GetHeight());
		camData.CameraData.row1.Set(_currentCamera->C()->As<Camera>()->GetNearZ(), _currentCamera->C()->As<Camera>()->GetFarZ(), 0.0f, 0.0f);
		cameraDataList.push_back(camData);

		static const glm::vec2 EnvCubeRotations[] =
		{
			glm::vec2(180.0f, 0.0f),
			glm::vec2(0.0f, 0.0f),
			glm::vec2(270.0f, 90.0f),
			glm::vec2(270.0f, -90.0f),
			glm::vec2(270.0f, 0.0f),
			glm::vec2(90.0f, 0.0f),
		};

		static float rotation = 0;
		//if (GraphicsWindow::KeyDown(KEY_V))
		//	rotation += 1.0f;;

		glm::mat4 envCubeProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.5f, 5000.0f);
		for (uint i = 0; i < 6; i++)
		{
			glm::mat4 envCubeView = glm::inverse(
				glm::rotate(Mat4::Identity, glm::radians(EnvCubeRotations[i].x + rotation), Vec3::Up) * glm::rotate(Mat4::Identity, glm::radians(EnvCubeRotations[i].y), Vec3::Right)
			);

			Shader::FillMatrices(envCubeView, envCubeProj, camData);
			camData.CameraData.row0.Set(0.0f, 0.0f, (float)_envTarget.Width(), (float)_envTarget.Height());
			camData.CameraData.row1.Set(_currentCamera->C()->As<Camera>()->GetNearZ(), _currentCamera->C()->As<Camera>()->GetFarZ(), 0.0f, 0.0f);
			cameraDataList.push_back(camData);
		}

		//glm::vec4 pos = glm::vec4(-1, 1, 0.99, 1);
		//glm::mat4 pixelMtx = Mat4::Identity;
		//pixelMtx[0][0] = pOutputTexture->GetWidth() / 2;
		//pixelMtx[1][1] = pOutputTexture->GetHeight() / 2;
		//pixelMtx[3][0] = pOutputTexture->GetWidth() / 2;
		//pixelMtx[3][1] = pOutputTexture->GetHeight() / 2;
		////pixelMtx = pixelMtx * proj;
		//pos = pixelMtx * pos;
		//pos /= pos.w;

		if (EngineInfo::GetRenderer().ShadowsEnabled())
		{
			_shadowCasterAABB.Reset();
			pScene->TraverseRenderNodes(
				[](const AABB&, void*) -> bool {
				return true;
			}, 0,
				[](RenderNode* pNode, void* pNodeData) -> void {	
				SceneRenderer* pThis = static_cast<SceneRenderer*>(pNodeData);
				if (pThis->ShouldRender(pNode))
				{
					const AABB& box = pNode->GetWorldAABB();
					if (pNode->GetMaterial()->GetShader()->ContainsVariants(ShaderVariant::DEPTH) /*&& !glm::epsilonEqual(box.Min.y, box.Max.y, 0.001f)*/)
						pThis->_shadowCasterAABB.Expand(box);
				}
			}, this);

			UpdateShadowCascades(cameraDataList);

			ThreadPool& tp = ThreadPool::Get();
			struct ThreadData
			{
				SceneRenderer* pThis;
				DepthRenderData* pDepthData;
				Scene* pScene;
			};

			Vector<ThreadData> threadDataList;
			threadDataList.resize(EngineInfo::GetRenderer().CascadeShadowMapSplits());
			for(uint i = 0; i < _depthPasses.size(); i++)
			{
				threadDataList[i].pDepthData = _depthPasses[i].get();
				threadDataList[i].pScene = pScene;
				threadDataList[i].pThis = this;

				tp.AddTask([](uint, void* pDataPtr) -> void 
				{
					ThreadData* pData = static_cast<ThreadData*>(pDataPtr);
					//ThreadData* pData = static_cast<ThreadData*>(&threadDataList[i]);
					pData->pScene->TraverseRenderNodes(
						[](const AABB& aabb, void* pAABBData) -> bool { 
							//return FrustumAABBIntersect(static_cast<const glm::vec4*>(pAABBData), aabb);
							return static_cast<DepthRenderData*>(pAABBData)->CameraData->FrustumIntersects(aabb);
							return true;
						}, pData->pDepthData,
						[](RenderNode* pNode, void* pNodeData) -> void { 
							ThreadData* pData = static_cast<ThreadData*>(pNodeData); 
							pData->pThis->ProcessDepthRenderNode(pNode, pData->pDepthData); 
						}, pData);
				}
				, & threadDataList[i]);
			}
			tp.Wait();

			for (auto& pass : _depthPasses)
			{
				for (auto& data : pass->RenderList)
				{
					auto pDepthMaterial = _depthMaterials[data.DepthHash].get();
					uint64 depthVariantMask = (data.BaseVariantMask & ~ShaderVariant::GBUFFER) | ShaderVariant::DEPTH;
					if (pDepthMaterial == 0)
					{
						pDepthMaterial = new Material();
						CreateDepthMaterial(data.RenderNode->GetMaterial(), depthVariantMask, pDepthMaterial);
						_depthMaterials[data.DepthHash] = UniquePtr<Material>(pDepthMaterial);
					}

					data.MaterialOverride = pDepthMaterial;
					bool sorted;
					GetPipeline(data, sorted, true, true);

					ObjectBufferData objBuffer = {};
					glm::mat4 itp = glm::transpose(glm::inverse(data.RenderNode->GetWorld()));
					objBuffer.WorldMatrix.Set(&data.RenderNode->GetWorld());
					objBuffer.InverseTransposeMatrix.Set(&itp);
					pass->ObjectBufferGroup.Update(&objBuffer, data.ObjectBufferIndex, &data.ObjectBindings, data.Pipeline->GetShader());

					//TODO skinned shadow objects
					if (PerformSkinningCheck(data.RenderNode))
						pass->SkinnedBonesBufferGroup.Update(_skinnedBoneMatrixBlock.data(), data.SkinnedBoneBufferIndex, &data.SkinnedBoneBindings, data.Pipeline->GetShader());

					_currentShaders.insert(data.Pipeline->GetShader());
				}

				pass->ObjectBufferGroup.Flush();
				pass->ObjectBufferGroup.Reset();

				pass->SkinnedBonesBufferGroup.Flush();
				pass->SkinnedBonesBufferGroup.Reset();
			}
		}

		pScene->TraverseRenderNodes(
			[](const AABB& aabb, void* pAABBData) -> bool { return static_cast<CameraComponentData*>(pAABBData)->FrustumIntersects(aabb); }, _currentCamera,
			[](RenderNode* pNode, void* pNodeData) -> void { static_cast<SceneRenderer*>(pNodeData)->ProcessRenderNode(pNode); }, this);

		//push current udpates to buffer
		_objectBufferGroup.Flush();
		_objectBufferGroup.Reset();

		_skinnedBonesBufferGroup.Flush();
		_skinnedBonesBufferGroup.Reset();

		_cameraBuffer->Buffer.UpdateShared(cameraDataList.data(), cameraDataList.size());

		Environment::FogSettings fog;
		_currentEnvironment->GetFogSettings(fog);
		fog.color = _currentEnvironment->GetActiveSkyModel()->GetSkyColor();

		EnvBufferData envData = {};
		glm::vec4 sunDir = glm::vec4(glm::normalize(_currentEnvironment->GetSunDirection()), 0.0);
		glm::vec4 sunDirView = _currentCamera->GetView() * sunDir;
		envData.SunColor.Set(1, 1, 1, 1);
		envData.SunDirection.Set(&sunDir.x);
		envData.SunViewDirection.Set(&sunDirView);
		envData.TimeData.Set(deltaTime, elapsedTime, 0, 0);
		envData.FogColor.Set(fog.color.x, fog.color.y, fog.color.z, 0.0f);
		envData.FogControls.Set(fog.enabled ? 1.0f : 0.0f, fog.heightFalloff, fog.density, 0.0f);
		if (!_environmentBuffer->Buffer.Update(&envData))
			return false;

		for (auto& pipeline : _helperPipelines)
			_currentShaders.insert(pipeline.second.GetShader());

		//plug in deferred shader so it gets camera/light buffers
		if (EngineInfo::GetRenderer().RenderMode() == EngineInfo::Renderer::Deferred)
		{
			_currentShaders.insert(ShaderMgr::Get().GetShader(DefaultShaders::Deferred)->GetBase());
			_currentShaders.insert(ShaderMgr::Get().GetShader(DefaultShaders::ScreenSpaceReflection)->GetBase());
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

			if ((pShader->ContainsBuffer(ShaderStrings::EnvBufferName) || pShader->ContainsResource(ShaderStrings::EnvTextureName)) && (_environmentBuffer->ShaderBindings.find(pShader) == _environmentBuffer->ShaderBindings.end()))
			{
				ShaderBindings::CreateInfo bindInfo = {};
				bindInfo.pShader = pShader;
				bindInfo.type = SBT_ENVIRONMENT;
				auto& binding = _environmentBuffer->ShaderBindings[pShader];
				if (!binding.Create(bindInfo))
					return false;
				binding.SetUniformBuffer(ShaderStrings::EnvBufferName, &_environmentBuffer->Buffer);
				binding.SetTexture(ShaderStrings::EnvTextureName, _envTarget.GetColorTexture());
				binding.SetSampler(ShaderStrings::EnvSamplerName, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE));
			}

			if (pShader->ContainsBuffer(ShaderStrings::ShadowBufferName) && _shadowBuffer->ShaderBindings.find(pShader) == _shadowBuffer->ShaderBindings.end())
			{
				ShaderBindings::CreateInfo bindInfo = {};
				bindInfo.pShader = pShader;
				bindInfo.type = SBT_SHADOW;
				auto& binding = _shadowBuffer->ShaderBindings[pShader];
				if (!binding.Create(bindInfo))
					return false;
				if (!binding.SetUniformBuffer(ShaderStrings::ShadowBufferName, &_shadowBuffer->Buffer))
					return false;
				bool depthTextureSet = binding.SetTexture(ShaderStrings::ShadowTextureName, EngineInfo::GetRenderer().ShadowsEnabled() ? _depthTarget.GetDepthTexture() : ResourceMgr::Get().GetTexture2D(DefaultResource::Texture::White)->GetGPUObject());
				bool depthSamplerSet = binding.SetSampler(ShaderStrings::ShadowSamplerName, ResourceMgr::Get().GetSampler(SE_FM_NEAREST, SE_WM_CLAMP_TO_BORDER, SE_BC_WHITE));
			}
		}

		return true;
	}

	bool SceneRenderer::RenderFrame(CommandBuffer* cmdBuffer, RenderTarget* pOpaqueTarget, RenderTargetPassInfo* pOutputInfo, DeferredRenderTargetPassInfo* pDeferredInfo, RenderTargetPassInfo* pMSAAResolveInfo)
	{
		if (!_currentCamera)
			return false;

		if (!_currentEnvironment)
			return false;

		for (uint i = 0; i < _depthPasses.size(); i++)
		{
			_depthTarget.BindLayer(cmdBuffer, i);
			ProcessRenderList(cmdBuffer, _depthPasses[i]->RenderList, _depthPasses[i]->CameraIndex, true);
			_depthPasses[i]->RenderList.clear();
			_depthTarget.Unbind(cmdBuffer);
		}

		RenderEnvironment(cmdBuffer);

		if (pMSAAResolveInfo)
		{
			pMSAAResolveInfo->pTarget->Bind(cmdBuffer);
			ProcessRenderList(cmdBuffer, _opaqueRenderList);
			RenderCommand(cmdBuffer, &_helperPipelines.at(HelperPipelines::EnvCopy), 0);
			pMSAAResolveInfo->pTarget->Unbind(cmdBuffer);
		}

		if (pDeferredInfo)
		{
			pDeferredInfo->pTarget->SetClearColor(0, 0, 0, 0);
			pDeferredInfo->pTarget->Bind(cmdBuffer);
			ProcessRenderList(cmdBuffer, _gbufferRenderList);
			pDeferredInfo->pTarget->Unbind(cmdBuffer);

			//Draw deferred objects to the deferred resolve render target
			pDeferredInfo->pDeferredResolveTarget->Bind(cmdBuffer);
			RenderCommand(cmdBuffer, pDeferredInfo->pPipeline, pDeferredInfo->pBindings);
			pDeferredInfo->pDeferredResolveTarget->Unbind(cmdBuffer);

			pOpaqueTarget->Bind(cmdBuffer);

			//copy deferred resolve to opaque render target
			RenderCommand(cmdBuffer, pDeferredInfo->pDeferredCopyPipeline, pDeferredInfo->pDeferredCopyBindings);

			////apply screen space reflections
			if (pDeferredInfo->pSSRPipeline)
			{
				RenderCommand(cmdBuffer, pDeferredInfo->pSSRPipeline, pDeferredInfo->pSSRBindings);
			}
		}
		else
		{
			pOpaqueTarget->Bind(cmdBuffer);
		}

		if (pMSAAResolveInfo)
		{
			//Copy opaque msaa to opaque
			RenderCommand(cmdBuffer, pMSAAResolveInfo->pPipeline, pMSAAResolveInfo->pBindings);
		}
		else
		{
			ProcessRenderList(cmdBuffer, _opaqueRenderList);
			RenderCommand(cmdBuffer, &_helperPipelines.at(HelperPipelines::EnvCopy), 0);
		}

		if(false)
		{
			IShaderBindingsBindState cameraBindData = {};
			cameraBindData.DynamicIndices[0] = { ShaderStrings::CameraBufferName, 0 };

			SkyModel* pSkyModel = _currentEnvironment->GetActiveSkyModel();
			Material* pMaterial = pSkyModel->GetMaterial();
			auto& pipeline = _helperPipelines.at(pMaterial->GetShader()->GetName());
			BaseShader* pShader = pipeline.GetShader();
			pShader->Bind(cmdBuffer);
			pipeline.Bind(cmdBuffer);
			pMaterial->GetGPUObject()->Bind(cmdBuffer);
			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer.get(), &cameraBindData);
			TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
			//screen quad default, is generated in shader
			cmdBuffer->Draw(6, 1, 0, 0);
			pMaterial->GetGPUObject()->Unbind(cmdBuffer);
			pipeline.Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);
		}

		pOpaqueTarget->Unbind(cmdBuffer);

		pOutputInfo->pTarget->Bind(cmdBuffer);
		if(true)
		{
			//Copy opaque to output
			RenderCommand(cmdBuffer, pOutputInfo->pPipeline, pOutputInfo->pBindings);
		}

		_sortedRenderList.sort([](const RenderNodeData& left, const RenderNodeData& right) -> bool { return left.SortingDistance > right.SortingDistance; });
		ProcessRenderList(cmdBuffer, _sortedRenderList);

		pOutputInfo->pTarget->Unbind(cmdBuffer);

		_currentShaders.clear();
		return true;
	}

	void SceneRenderer::ProcessRenderNode(RenderNode* pNode)
	{
		Material* pMaterial = pNode->GetMaterial();
		if (!ShouldRender(pNode))
			return;

		SceneNode* pSceneNode = pNode->GetNode();
		//if (!StrContains(pSceneNode->GetName(), "Frustum"))
		{
			if (!_currentCamera->FrustumIntersects(pNode->GetWorldAABB()))
				return;
		}


		bool sorted = false;
		RenderNodeData data = {};
		data.RenderNode = pNode;
		data.BaseVariantMask = GetVariantMask(data.RenderNode);
		GetPipeline(data, sorted);

		BaseShader* pShader = pMaterial->GetShader()->GetBaseVariant(data.BaseVariantMask);

		ObjectBufferData objBuffer = {};
		glm::mat4 itp = glm::transpose(glm::inverse(pNode->GetWorld()));
		objBuffer.WorldMatrix.Set(&pNode->GetWorld());
		objBuffer.InverseTransposeMatrix.Set(&itp);

		_objectBufferGroup.Update(&objBuffer, data.ObjectBufferIndex, &data.ObjectBindings, pShader);

		if (PerformSkinningCheck(pNode))
			_skinnedBonesBufferGroup.Update(_skinnedBoneMatrixBlock.data(), data.SkinnedBoneBufferIndex, &data.SkinnedBoneBindings, pShader);

		_currentShaders.insert(pShader);

		if (!sorted)
		{
			if(data.BaseVariantMask & ShaderVariant::GBUFFER)
				_gbufferRenderList.push_back(data);
			else
				_opaqueRenderList.push_back(data);
		}
		else
		{
			glm::vec3 vDelta = pNode->GetWorldAABB().GetCenter() - _currentCamera->GetPosition();
			data.SortingDistance = glm::dot(vDelta, vDelta);
			_sortedRenderList.push_back(data);
		}
	}

	void SceneRenderer::ProcessDepthRenderNode(RenderNode* pNode, DepthRenderData* pDepthData)
	{
		if (!ShouldRender(pNode))
			return;

		const AABB& box = pNode->GetWorldAABB();

		//cheap attempt to not have plane like surfaces cast shadows
		if(glm::epsilonEqual(box.Min.y, box.Max.y, 0.001f))
			return;

		//if (!pDepthData->FrustumBox.Contains(box))
		//	return;

		if (!pDepthData->CameraData->FrustumIntersects(pNode->GetWorldAABB()))
			return;

		Material* pMaterial = pNode->GetMaterial();
		uint64 variantMask = GetVariantMask(pNode);
		uint64 depthVariantMask = (variantMask & ~ShaderVariant::GBUFFER) | ShaderVariant::DEPTH;

		BaseShader* pDepthShader = pMaterial->GetShader()->GetBaseVariant(depthVariantMask);
		if (pDepthShader)
		{
			RenderNodeData depthNode = {};
			depthNode.DepthHash = CalculateDepthVariantHash(pMaterial, depthVariantMask);
			depthNode.BaseVariantMask = variantMask;
			depthNode.RenderNode = pNode;
			pDepthData->RenderList.push_back(depthNode);
		}
	}

	void SceneRenderer::ProcessRenderList(CommandBuffer* cmdBuffer, LinkedList<RenderNodeData>& renderList, uint cameraUpdateIndex, bool isDepth)
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
			GraphicsPipeline& pipeline = *renderData.Pipeline;
			BaseShader* pShader = pipeline.GetShader();

			pShader->Bind(cmdBuffer);
			pipeline.Bind(cmdBuffer);
			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer.get(), &cameraBindData);
			TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
			TryBindBuffer(cmdBuffer, pShader, _shadowBuffer.get());

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

	bool SceneRenderer::GetPipeline(RenderNodeData& data, bool& sorted, bool isDepth, bool isShadow)
	{
		sorted = false;
		PipelineSettings settings = {};
		data.RenderNode->BuildPipelineSettings(settings);

		uint64 variantMask = data.BaseVariantMask;
		if (isDepth)
		{
			variantMask &= ~ShaderVariant::GBUFFER;
			variantMask |= ShaderVariant::DEPTH;
		}

		BaseShader* pShader = data.RenderNode->GetMaterial()->GetShader()->GetBaseVariant(variantMask);

		float opacity = 1.0f;
		if (!isDepth && data.RenderNode->GetMaterial()->GetMaterialVar(MaterialStrings::Opacity, opacity) && opacity < 1.0f)
		{
			settings.EnableAlphaBlend();
			sorted = true;
		}
		else if (variantMask & ShaderVariant::ALPHA_TEST)
		{
			settings.rasterizer.cullMode = SE_CM_NONE; //TODO: should this be set somehow else
			settings.mulitSampleState.enableAlphaToCoverage = (isDepth || isShadow) ? false : true; //TODO: only do this if multi sampling is occuring in the renderer this frame?
		}

		if (isShadow) ShaderMgr::Get().BuildPipelineSettings(DefaultPipelines::ShadowDepth, settings);

#ifdef GLM_FORCE_LEFT_HANDED
		settings.rasterizer.frontFace = SE_FF_CLOCKWISE;
#endif

		//if (data.RenderNode->GetMaterial()->GetShader()->GetName() == DefaultShaders::Terrain) settings.rasterizer.polygonMode = SE_PM_LINE;

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

	bool SceneRenderer::TryBindBuffer(CommandBuffer* cmdBuffer, BaseShader* pShader, UniformBufferData* buffer, IBindState* pBindState) const
	{
		auto found = buffer->ShaderBindings.find(pShader);
		if(found != buffer->ShaderBindings.end())
			return (*found).second.Bind(cmdBuffer, pBindState);
		return false;
	}

	//void SceneRenderer::RenderSky(CommandBuffer* cmdBuffer)
	//{
	//	IShaderBindingsBindState cameraBindData = {};
	//	cameraBindData.DynamicIndices[0] = { ShaderStrings::CameraBufferName, 0 };

	//	SkyModel* pSkyModel = _currentEnvironment->GetActiveSkyModel();
	//	Material* pMaterial = pSkyModel->GetMaterial();
	//	auto& pipeline = _helperPipelines.at(pMaterial->GetShader()->GetName());
	//	BaseShader* pShader = pipeline.GetShader();
	//	SkyModel::MeshType meshType = pSkyModel->GetMeshType();

	//	Mesh* pMesh = 0;
	//	switch (meshType)
	//	{
	//	case SunEngine::SkyModel::MT_CUBE:
	//		pMesh = ResourceMgr::Get().GetMesh(DefaultResource::Mesh::Cube);
	//		break;
	//	case SunEngine::SkyModel::MT_SPHERE:
	//		pMesh = ResourceMgr::Get().GetMesh(DefaultResource::Mesh::Sphere);
	//		break;
	//	case SunEngine::SkyModel::MT_QUAD:
	//		pMesh = ResourceMgr::Get().GetMesh(DefaultResource::Mesh::Quad);
	//		break;
	//	default:
	//		break;
	//	}

	//	pShader->Bind(cmdBuffer);
	//	pipeline.Bind(cmdBuffer);
	//	pMaterial->GetGPUObject()->Bind(cmdBuffer);
	//	TryBindBuffer(cmdBuffer, pShader, _cameraBuffer.get(), &cameraBindData);
	//	TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
	//	if (pMesh)
	//	{
	//		pMesh->GetGPUObject()->Bind(cmdBuffer);
	//		cmdBuffer->DrawIndexed(pMesh->GetIndexCount(), 1, 0, 0, 0);
	//	}
	//	else
	//	{
	//		//screen quad default, is generated in shader
	//		cmdBuffer->Draw(6, 1, 0, 0);
	//	}
	//	pMaterial->GetGPUObject()->Unbind(cmdBuffer);
	//	pipeline.Unbind(cmdBuffer);
	//	pShader->Unbind(cmdBuffer);
	//}

	void SceneRenderer::RenderEnvironment(CommandBuffer* cmdBuffer)
	{
		static const glm::vec3 CubeColors[] =
		{
			glm::vec3(1.0f, 0.0f, 0.0f), //right
			glm::vec3(0.0f, 1.0f, 0.0f), //left
			glm::vec3(0.0f, 0.0f, 0.0f), //up
			glm::vec3(1.0f, 1.0f, 1.0f), //down
			glm::vec3(1.0f, 1.0f, 0.0f), //forward
			glm::vec3(0.0f, 0.0f, 1.0f), //back
		};


		IShaderBindingsBindState cameraBindData = {};
		cameraBindData.DynamicIndices[0] = { ShaderStrings::CameraBufferName, 0 };

		SkyModel* pSkyModel = _currentEnvironment->GetActiveSkyModel();
		Material* pMaterial = pSkyModel->GetMaterial();
		auto& pipeline = _helperPipelines.at(pMaterial->GetShader()->GetName());
		BaseShader* pShader = pipeline.GetShader();

		for (uint i = 0; i < 6; i++)
		{
			_envTarget.SetClearColor(CubeColors[i].r, CubeColors[i].g, CubeColors[i].b, 1.0f);
			_envTarget.BindLayer(cmdBuffer, i);

			cameraBindData.DynamicIndices[0].second = i + 1; //assumes that camera ubo was updated with env cameras right after main camera

			pShader->Bind(cmdBuffer);
			pipeline.Bind(cmdBuffer);
			pMaterial->GetGPUObject()->Bind(cmdBuffer);
			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer.get(), &cameraBindData);
			TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
			//screen quad default, is generated in shader
			cmdBuffer->Draw(6, 1, 0, 0);
			pMaterial->GetGPUObject()->Unbind(cmdBuffer);
			pipeline.Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);

			//Draw clouds over sky
			RenderCommand(cmdBuffer, &_helperPipelines.at(DefaultShaders::Clouds), _currentEnvironment->GetCloudBindings(), 6, cameraBindData.DynamicIndices[0].second);

			_envTarget.Unbind(cmdBuffer);
		}

		////Copy the texture containing the sky background to main framebuffer
		//RenderCommand(cmdBuffer, &_helperPipelines.at(HelperPipelines::SkyCopy), &_skyBindings);
	}

	void SceneRenderer::RenderCommand(CommandBuffer* cmdBuffer, GraphicsPipeline* pPipeline, ShaderBindings* pBindings, uint vertexCount, uint cameraUpdateIndex)
	{
		IShaderBindingsBindState cameraBindData = {};
		cameraBindData.DynamicIndices[0] = { ShaderStrings::CameraBufferName, cameraUpdateIndex };

		BaseShader* pShader = pPipeline->GetShader();
		pShader->Bind(cmdBuffer);
		pPipeline->Bind(cmdBuffer);
		if(pBindings) pBindings->Bind(cmdBuffer);
		TryBindBuffer(cmdBuffer, pShader, _cameraBuffer.get(), &cameraBindData);
		TryBindBuffer(cmdBuffer, pShader, _environmentBuffer.get());
		TryBindBuffer(cmdBuffer, pShader, _shadowBuffer.get());
		cmdBuffer->Draw(vertexCount, 1, 0, 0);
		if(pBindings) pBindings->Unbind(cmdBuffer);
		pPipeline->Unbind(cmdBuffer);
		pShader->Unbind(cmdBuffer);
	}

	bool SceneRenderer::CreateDepthMaterial(Material* pMaterial, uint64 variantMask, Material* pEmptyMaterial) const
	{
		pEmptyMaterial->SetShader(pMaterial->GetShader(), variantMask);
		if (!pEmptyMaterial->RegisterToGPU())
			return false;

		StrMap<ShaderProp>* depthVariantProps;
		pMaterial->GetShader()->GetVariantProps(variantMask, &depthVariantProps);
		for (auto& prop : *depthVariantProps)
		{
			switch (prop.second.Type)
			{
			case SPT_TEXTURE2D:
				pEmptyMaterial->SetTexture2D(prop.first, pMaterial->GetTexture2D(prop.first));
				break;
			case SPT_SAMPLER:
				pEmptyMaterial->SetSampler(prop.first, prop.second.pSampler);
				break;
			case SPT_FLOAT:
			{
				float val;
				pMaterial->GetMaterialVar(prop.first, val);
				pEmptyMaterial->SetMaterialVar(prop.first, val);
			} break;
			case SPT_FLOAT2:
			{
				glm::vec2 val;
				pMaterial->GetMaterialVar(prop.first, val);
				pEmptyMaterial->SetMaterialVar(prop.first, val);
			} break;
			case SPT_FLOAT3:
			{
				glm::vec3 val;
				pMaterial->GetMaterialVar(prop.first, val);
				pEmptyMaterial->SetMaterialVar(prop.first, val);
			} break;
			case SPT_FLOAT4:
			{
				glm::vec4 val;
				pMaterial->GetMaterialVar(prop.first, val);
				pEmptyMaterial->SetMaterialVar(prop.first, val);
			} break;
			default:
				break;
			}
		}

		return true;
	}

	uint64 SceneRenderer::CalculateDepthVariantHash(Material* pMaterial, uint64 variantMask) const
	{
		uint64 hash = variantMask;

		StrMap<ShaderProp>* depthVariantProps;
		pMaterial->GetShader()->GetVariantProps(variantMask, &depthVariantProps);
		for (auto& prop : *depthVariantProps)
		{
			uint64 keyHash = std::hash<String>()(prop.first);
			uint64 valHash = 0;
			switch (prop.second.Type)
			{
			case SPT_TEXTURE2D:
				valHash = (uint64)pMaterial->GetTexture2D(prop.first);
				break;
			case SPT_SAMPLER:
				valHash = (uint64)prop.second.pSampler;
				break;
			case SPT_FLOAT:
			{
				float val;
				pMaterial->GetMaterialVar(prop.first, val);
				valHash = std::hash<float>()(val);
			} break;
			case SPT_FLOAT2:
			{
				glm::vec2 val;
				pMaterial->GetMaterialVar(prop.first, val);
				valHash = std::hash<glm::vec2>()(val);
			} break;
			case SPT_FLOAT3:
			{
				glm::vec3 val;
				pMaterial->GetMaterialVar(prop.first, val);
				valHash = std::hash<glm::vec3>()(val);
			} break;
			case SPT_FLOAT4:
			{
				glm::vec4 val;
				pMaterial->GetMaterialVar(prop.first, val);
				valHash = std::hash<glm::vec4>()(val);
			} break;
			default:
				break;
			}

			hash += keyHash << 2;
			hash += valHash >> 2;
		}

		return hash;
	}

	void SceneRenderer::UpdateShadowCascades(Vector<CameraBufferData>& cameraBuffersToFill)
	{
		if (!EngineInfo::GetRenderer().ShadowsEnabled())
			return;

		uint cascadeCount = EngineInfo::GetRenderer().CascadeShadowMapSplits();
		if (cascadeCount == 0)
			return;

		ShadowBufferData shadowBufferData;
		static const glm::mat4 ShadowBiasMtx = glm::translate(Mat4::Identity, glm::vec3(0.5f, 0.5f, 0.0f)) * glm::scale(Mat4::Identity, glm::vec3(0.5f, -0.5f, 1.0f));
#if 1
		glm::mat4 lightViewMatrix;
		static Camera shadowCameras[ShadowBufferData::MAX_CASCADE_SPLITS];
		float shadowCascadeSplits[ShadowBufferData::MAX_CASCADE_SPLITS];
		CascadedShadowMap::UpdateInfo csmInfo = {};
		csmInfo.cascadeFitMode = CascadedShadowMap::FIT_TO_SCENE;
		csmInfo.nearFarFitMode = CascadedShadowMap::FIT_NEARFAR_SCENE_AABB;
		csmInfo.lightPos = _currentEnvironment->GetSunDirection();
		csmInfo.pCameraData = _currentCamera;
		csmInfo.sceneBounds = _shadowCasterAABB;
		csmInfo.cascadeSplitLambda = _cascadeSplitLambda;
		CascadedShadowMap::Update(csmInfo, lightViewMatrix, shadowCameras, shadowCascadeSplits);

		for (uint i = 0; i < cascadeCount; i++)
		{
			auto pDepthPass = _depthPasses[i].get();

			CameraBufferData cameraBuffer = {};
			Shader::FillMatrices(lightViewMatrix, shadowCameras[i].GetProj(), cameraBuffer);
			pDepthPass->CameraIndex = cameraBuffersToFill.size();
			cameraBuffersToFill.push_back(cameraBuffer);

			glm::mat4 shadowMatrix = ShadowBiasMtx * reinterpret_cast<glm::mat4&>(cameraBuffer.ViewProjectionMatrix);
			shadowBufferData.ShadowMatrices[i].Set(&shadowMatrix);
			shadowBufferData.ShadowSplitDepths.Set(i, -shadowCascadeSplits[i]);

			Camera& camera = shadowCameras[i];

			SceneNode node(0);
			glm::mat4 invView = reinterpret_cast<glm::mat4&>(cameraBuffer.InvViewMatrix);
			node.Position = invView[3];
			node.Orientation.Quat = glm::quat_cast(invView);
			node.Orientation.Mode = ORIENT_QUAT;
			node.UpdateTransform();

			camera.Update(&node, pDepthPass->CameraData.get(), 0, 0);
			pDepthPass->FrustumBox.Reset();
			for (uint j = 0; j < 8; j++)
				pDepthPass->FrustumBox.Expand(pDepthPass->CameraData->GetFrustumCorner(j));
		}
		_shadowBuffer->Buffer.Update(&shadowBufferData);
#else

		uint SHADOW_MAP_CASCADE_COUNT = EngineInfo::GetRenderer().CascadeShadowMapSplits();
		if (SHADOW_MAP_CASCADE_COUNT == 0)
			return;

		const Camera* pCamera = _currentCamera->C()->As<const Camera>();



#if 1
		/*
			Calculate frustum split depths and matrices for the shadow map cascades
			Based on https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
		*/
		float cascadeSplits[SE_ARR_SIZE(shadowBufferData.ShadowMatrices)];

		float nearClip = pCamera->GetNearZ();
		float farClip = pCamera->GetFarZ();
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = _cascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t s = 0; s < SHADOW_MAP_CASCADE_COUNT; s++) {
			float splitDist = cascadeSplits[s];

			glm::vec3 frustumCorners[8] = {
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(pCamera->GetProj() * _currentCamera->GetView());
			for (uint32_t i = 0; i < 8; i++) {
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++) {
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++) {
				frustumCenter += frustumCorners[i];
			}
			frustumCenter /= 8.0f;

			auto* pDepthPass = _depthPasses[s].get();

			AABB test;
			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++) {
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
				test.Expand(frustumCorners[i]);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -glm::normalize(_currentEnvironment->GetSunDirection());
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

			// Store split distance and matrix in cascade
			float splitDepth = (pCamera->GetNearZ() + splitDist * clipRange) * -1.0f;
			glm::mat4 viewProjMatrix = lightOrthoMatrix * lightViewMatrix;

			CameraBufferData shadowCamData;
			shadowCamData.CameraData.row0.Set(pDepthPass->Viewport.x, pDepthPass->Viewport.y, pDepthPass->Viewport.width, pDepthPass->Viewport.height);
			Shader::FillMatrices(lightViewMatrix, lightOrthoMatrix, shadowCamData);
			cameraBuffersToFill.push_back(shadowCamData);

			viewProjMatrix = ShadowBiasMtx * viewProjMatrix;
			shadowBufferData.ShadowMatrices[s].Set(&viewProjMatrix);
			shadowBufferData.ShadowSplitDepths.Set(s, splitDepth);
			lastSplitDist = cascadeSplits[s];

			//glm::vec3(-1.0f, 1.0f, -1.0f),
			//glm::vec3(1.0f, 1.0f, -1.0f),
			//glm::vec3(1.0f, -1.0f, -1.0f),
			//glm::vec3(-1.0f, -1.0f, -1.0f),
			//glm::vec3(-1.0f, 1.0f, 1.0f),
			//glm::vec3(1.0f, 1.0f, 1.0f),
			//glm::vec3(1.0f, -1.0f, 1.0f),
			//glm::vec3(-1.0f, -1.0f, 1.0f),

			Camera camera;
			SceneNode node(0);

			camera.SetOrthoProjection(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);
			glm::mat4 invView = reinterpret_cast<glm::mat4&>(shadowCamData.InvViewMatrix);

			node.Position = invView[3];
			node.Orientation.Quat = glm::quat_cast(invView);
			node.Orientation.Mode = ORIENT_QUAT;
			node.UpdateTransform();

			camera.Update(&node, pDepthPass->CameraData.get(), 0, 0);
			pDepthPass->FrustumBox.Reset();
			for (uint i = 0; i < 8; i++)
				pDepthPass->FrustumBox.Expand(pDepthPass->CameraData->GetFrustumCorner(i));

			pDepthPass->FrustumBox.Expand(glm::vec3(FLT_MAX));
			pDepthPass->FrustumBox.Expand(glm::vec3(-FLT_MAX));
		}
#else

#endif

		_shadowBuffer->Buffer.Update(&shadowBufferData);
#endif
	}

	bool SceneRenderer::ShouldRender(const RenderNode* pNode) const
	{
		Material* pMaterial = pNode->GetMaterial();
		return pMaterial && pMaterial->GetShader() && pNode->GetMesh() && pNode->GetNode()->GetTotalVisibility();
	}

	uint64 SceneRenderer::GetVariantMask(const RenderNode* pNode) const
	{
		uint64 variantMask = 0;
		if (pNode->GetNode()->GetComponentOfType(COMPONENT_SKINNED_MESH))
			variantMask |= ShaderVariant::SKINNED;

		Material* pMaterial = pNode->GetMaterial();

		Texture2D* pAlphaTex = pMaterial->GetTexture2D(MaterialStrings::AlphaMap);
		if (pAlphaTex && !ResourceMgr::Get().IsDefaultTexture2D(pAlphaTex))
			variantMask |= ShaderVariant::ALPHA_TEST;

		if (EngineInfo::GetRenderer().RenderMode() == EngineInfo::Renderer::Deferred)
			variantMask = pMaterial->GetShader()->GetBaseVariant(variantMask | ShaderVariant::GBUFFER) ? (variantMask | ShaderVariant::GBUFFER) : variantMask;

		return variantMask;
	}

	bool SceneRenderer::PerformSkinningCheck(const RenderNode* pNode)
	{
		SceneNode* pSceneNode = pNode->GetNode();

		Component* pSMComponent = pSceneNode->GetComponentOfType(COMPONENT_SKINNED_MESH, [](const Component* pComponent, void* pData) -> bool {
			return pComponent->As<SkinnedMesh>()->GetMesh() == pData;
		}, pNode->GetMesh());

		if (pSMComponent)
		{
			SkinnedMeshComponentData* pSkinnedData = pSceneNode->GetComponentData<SkinnedMeshComponentData>(pSMComponent);
			AnimatorComponentData* pAnimatorData = pSkinnedData->GetAnimatorData();

			if (pAnimatorData->GetPlaying())
			{
				const glm::mat4* pMatrices = pSkinnedData->GetMeshBoneMatrices();
				for (uint i = 0; i < pAnimatorData->GetBoneCount(); i++)
					_skinnedBoneMatrixBlock[i].Set(&pMatrices[i]);
			}
			else
			{
				for (uint i = 0; i < pAnimatorData->GetBoneCount(); i++)
					_skinnedBoneMatrixBlock[i].Set(&Mat4::Identity);
			}

			return true;
		}
		else
		{
			return false;
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

	SceneRenderer::DepthRenderData::DepthRenderData()
	{
		CameraData = UniquePtr<CameraComponentData>(new CameraComponentData());
	}

}