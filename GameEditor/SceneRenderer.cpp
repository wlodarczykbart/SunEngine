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

#include <DirectXMath.h>

namespace SunEngine
{
	SceneRenderer::SceneRenderer()
	{
		_bInit = false;
		_currentCamera = 0;
		_currentSunlight = 0;
		_lightBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
		_shadowMatrixBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	bool SceneRenderer::Init()
	{
		UniformBuffer::CreateInfo uboInfo = {};
		uboInfo.isShared = false;

		uboInfo.size = sizeof(SunlightBufferData);
		if (!_lightBuffer->Buffer.Create(uboInfo))
			return false;

		if (!_cameraGroup.Init())
			return false;

		if (!_objectBufferGroup.Init())
			return false;

		if (EngineInfo::GetRenderer().ShadowsEnabled())
		{
			_depthPasses.resize(EngineInfo::GetRenderer().MaxShadowCascadeSplits());
			uint depthSliceSize = EngineInfo::GetRenderer().CascadeShadowMapResolution();

			RenderTarget::CreateInfo depthTargetInfo = {};
			depthTargetInfo.hasDepthBuffer = true;
			depthTargetInfo.floatingPointColorBuffer = true;
			depthTargetInfo.width = depthSliceSize * _depthPasses.size();
			depthTargetInfo.height = depthSliceSize;
			depthTargetInfo.numTargets = 1;

			if (!_depthTarget.Create(depthTargetInfo))
				return false;

			float vpWidth = (float)depthSliceSize / depthTargetInfo.width;
			float vpOffset = 0.0f;

			for (uint i = 0; i < _depthPasses.size(); i++)
			{
				_depthPasses[i] = UniquePtr<DepthRenderData>(new DepthRenderData());
				if (!_depthPasses[i]->ObjectBufferGroup.Init())
					return false;

				_depthPasses[i]->Viewport = { vpOffset, 0.0f, vpWidth, 1.0f };
				vpOffset += vpWidth;
			}

			uboInfo.size = sizeof(glm::mat4) * _depthPasses.size();
			if (!_shadowMatrixBuffer->Buffer.Create(uboInfo))
				return false;
		}

		_shaderVariantPipelineMap[Shader::Depth] = DefaultPipelines::ShadowDepth;

		_bInit = true;
		return true;
	}

	bool SceneRenderer::PrepareFrame(CameraComponentData* pCamera)
	{
		if (!_bInit)
			return false;

		Scene* pScene = SceneMgr::Get().GetActiveScene();
		if (!pScene)
			return false;

		_currentCamera = pCamera;
		_currentSunlight = 0;

		pScene->Traverse(TraverseFunc, this);

		//push current udpates to buffer
		_objectBufferGroup.Flush();
		_objectBufferGroup.Reset();

		for (uint i = 0; i < _depthPasses.size(); i++)
		{
			_depthPasses[i]->ObjectBufferGroup.Flush();
			_depthPasses[i]->ObjectBufferGroup.Reset();
		}

		if (!_currentCamera)
			return false;

		if (!_currentSunlight)
			return false;

		CameraBufferData camData = {};
		glm::mat4 view = _currentCamera->ViewMatrix;
		glm::mat4 proj = _currentCamera->C()->As<Camera>()->GetProj();
		glm::mat4 invView = glm::inverse(view);
		glm::mat4 invProj = glm::inverse(proj);
		camData.ViewMatrix.Set(&view);
		camData.ProjectionMatrix.Set(&proj);
		camData.InvViewMatrix.Set(&invView);
		camData.InvProjectionMatrix.Set(&invProj);

		uint camUpdateIndex = 0;
		_cameraGroup.Update(camData, camUpdateIndex, &_cameraBuffer);

		glm::mat4 shadowMatrices[16];
		for (uint i = 0; i < _depthPasses.size(); i++)
		{
			float sceneRadius = 20.0f;
			glm::vec3 lightDir = glm::normalize(glm::vec3(_currentSunlight->Direction));
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

			invView = glm::inverse(view);
			invProj = glm::inverse(proj);
			camData.ViewMatrix.Set(&view);
			camData.ProjectionMatrix.Set(&proj);
			camData.InvViewMatrix.Set(&invView);
			camData.InvProjectionMatrix.Set(&invProj);
			_cameraGroup.Update(camData, camUpdateIndex, &_cameraBuffer);
			glm::mat4 viewProj = proj * view;
			_shadowMatrixBuffer->Buffer.Update(&viewProj, i * sizeof(glm::mat4), sizeof(glm::mat4));
		}

		_cameraGroup.Flush();
		_cameraGroup.Reset();

		SunlightBufferData sunData = {};
		glm::vec4 sunDirView = _currentCamera->ViewMatrix * _currentSunlight->Direction;
		sunData.Color.Set(&_currentSunlight->C()->As<Light>()->GetColor());
		sunData.Direction.Set(&_currentSunlight->Direction);
		sunData.ViewDirection.Set(&sunDirView);
		if (!_lightBuffer->Buffer.Update(&sunData))
			return false;

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

			if (pShader->ContainsBuffer(ShaderStrings::SunlightBufferName) && _lightBuffer->ShaderBindings.find(pShader) == _lightBuffer->ShaderBindings.end())
			{
				ShaderBindings::CreateInfo bindInfo = {};
				bindInfo.pShader = pShader;
				bindInfo.type = SBT_LIGHT;
				if (!_lightBuffer->ShaderBindings[pShader].Create(bindInfo))
					return false;
				if (!_lightBuffer->ShaderBindings[pShader].SetUniformBuffer(ShaderStrings::SunlightBufferName, &_lightBuffer->Buffer))
					return false;
			}

			if (_depthPasses.size())
			{
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
					bool depthTextureSet = binding.SetTexture(ShaderStrings::ShadowTextureName, _depthTarget.GetDepthTexture());
					bool depthSamplerSet = binding.SetSampler(ShaderStrings::ShadowSamplerName, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_BORDER, SE_BC_WHITE));
				}
			}
		}

		return true;
	}

	bool SceneRenderer::RenderFrame(CommandBuffer* cmdBuffer, RenderTarget* pOpaqueTarget, RenderTargetPassInfo* pOutputInfo, DeferredRenderTargetPassInfo* pDeferredInfo)
	{
		if (!_currentCamera)
			return false;

		if (!_currentSunlight)
			return false;

		BaseShader* pShader = 0;

		_depthTarget.Bind(cmdBuffer);
		for (uint i = 0; i < _depthPasses.size(); i++)
		{
			ProcessRenderQueue(cmdBuffer, _depthPasses[i]->RenderQueue, i+1);
		}
		_depthTarget.Unbind(cmdBuffer);

		uint mainCamUpdateIndex = 0;

		if (pDeferredInfo)
		{
			pDeferredInfo->pTarget->Bind(cmdBuffer);
			ProcessRenderQueue(cmdBuffer, _gbufferRenderQueue, mainCamUpdateIndex);
			pDeferredInfo->pTarget->Unbind(cmdBuffer);

			//Draw deferred objects to the deferred resolve render target
			pDeferredInfo->pDeferredResolveTarget->Bind(cmdBuffer);

			pShader = pDeferredInfo->pPipeline->GetShader();
			pShader->Bind(cmdBuffer);
			pDeferredInfo->pPipeline->Bind(cmdBuffer);
			pDeferredInfo->pBindings->Bind(cmdBuffer);

			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer);
			TryBindBuffer(cmdBuffer, pShader, _lightBuffer.get());
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

		ProcessRenderQueue(cmdBuffer, _opaqueRenderQueue, mainCamUpdateIndex);

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

		ProcessRenderQueue(cmdBuffer, _sortedRenderQueue, mainCamUpdateIndex);

		pOutputInfo->pTarget->Unbind(cmdBuffer);

		_currentShaders.clear();
		return true;
	}

	void SceneRenderer::TraverseFunc(SceneNode* pNode, void* pUserData)
	{
		static_cast<SceneRenderer*>(pUserData)->ProcessNode(pNode);
	}

	void SceneRenderer::ProcessNode(SceneNode* pNode)
	{
		bool canRender = pNode->CanRender();
		uint nCameras = pNode->GetComponentCount(COMPONENT_CAMERA);
		uint nLights = pNode->GetComponentCount(COMPONENT_LIGHT);

		if (!canRender && nCameras == 0 && nLights == 0) return;

		for (auto iter = pNode->BeginComponent(); iter != pNode->EndComponent(); ++iter)
		{
			Component* component = (*iter);

			ComponentType type = component->GetType();
			if(_currentCamera == 0 && type == COMPONENT_CAMERA)
			{
				if (component->As<Camera>()->GetRenderToWindow())
					_currentCamera = pNode->GetComponentData<CameraComponentData>(component);
			}
			else if (_currentSunlight == 0 && type == COMPONENT_LIGHT)
			{
				if (component->As<Light>()->GetLightType() == LT_DIRECTIONAL)
					_currentSunlight = pNode->GetComponentData<LightComponentData>(component);
			}
			else if (component->CanRender())
			{
				RenderComponentData* pRenderData = pNode->GetComponentData<RenderComponentData>(component);
				for (auto renderIter = pRenderData->BeginNode(); renderIter != pRenderData->EndNode(); ++renderIter)
				{
					const RenderNode& renderNode = *(renderIter);
					Material* pMaterial = renderNode.GetMaterial();
					bool isValid = pMaterial && pMaterial->GetShader() && renderNode.GetMesh();

					if (isValid)
					{
						RenderNodeData data = {};
						data.SceneNode = pNode;
						data.RenderNode = &renderNode;
						GetPipeline(data);

						BaseShader* pShader = pMaterial->GetShaderVariant();

						ObjectBufferData objBuffer = {};
						glm::mat4 itp = glm::transpose(glm::inverse(renderNode.GetWorld()));
						objBuffer.WorldMatrix.Set(&renderNode.GetWorld());
						objBuffer.InverseTransposeMatrix.Set(&itp);

						_objectBufferGroup.Update(objBuffer, data.ObjectBufferIndex, &data.ObjectBindings, pShader);
						_currentShaders.insert(pShader);

						if (pMaterial->GetVariant() == Shader::Default)
						{
							//TODO: add logic to place these in either opaque or sorted based on the render node properties
							_sortedRenderQueue.push(data);
						}
						else if (pMaterial->GetVariant() == Shader::GBuffer)
						{
							_gbufferRenderQueue.push(data);
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
									GetPipeline(depthData);
									_depthPasses[i]->ObjectBufferGroup.Update(objBuffer, depthData.ObjectBufferIndex, &depthData.ObjectBindings, pDepthShader);
									_depthPasses[i]->RenderQueue.push(depthData);
								}

								_currentShaders.insert(pDepthShader);
							}
						}
					}
				}
			}
		}
	}

	void SceneRenderer::ProcessRenderQueue(CommandBuffer* cmdBuffer, Queue<RenderNodeData>& queue, uint cameraUpdateIndex)
	{
		IShaderBindingsBindState objectBindData = {};
		objectBindData.DynamicIndices[0].first = ShaderStrings::ObjectBufferName;

		IShaderBindingsBindState cameraBindData = {};
		cameraBindData.DynamicIndices[0] = { ShaderStrings::CameraBufferName, cameraUpdateIndex };

		while (queue.size())
		{
			RenderNodeData& renderData = queue.front();
			Material* pMaterial = renderData.MaterialOverride ? renderData.MaterialOverride : renderData.RenderNode->GetMaterial();

			BaseShader* pShader = pMaterial->GetShaderVariant();
			GraphicsPipeline& pipeline = *renderData.Pipeline;

			//pipeline.GetShader()->getgpu
			pShader->Bind(cmdBuffer);
			pipeline.Bind(cmdBuffer);
			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer, &cameraBindData);
			TryBindBuffer(cmdBuffer, pShader, _lightBuffer.get());
			TryBindBuffer(cmdBuffer, pShader, _shadowMatrixBuffer.get());

			renderData.RenderNode->GetMesh()->GetGPUObject()->Bind(cmdBuffer);
			pMaterial->GetGPUObject()->Bind(cmdBuffer);

			objectBindData.DynamicIndices[0].second = renderData.ObjectBufferIndex;
			renderData.ObjectBindings->ShaderBindings.at(pShader).Bind(cmdBuffer, &objectBindData);

			cmdBuffer->DrawIndexed(
				renderData.RenderNode->GetIndexCount(),
				renderData.RenderNode->GetInstanceCount(),
				renderData.RenderNode->GetFirstIndex(),
				renderData.RenderNode->GetVertexOffset(),
				0);

			pipeline.Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);

			queue.pop();
		}
	}

	bool SceneRenderer::GetPipeline(RenderNodeData& data)
	{
		PipelineSettings settings = {};
		data.RenderNode->BuildPipelineSettings(settings);

		Material* pMaterial = data.MaterialOverride ? data.MaterialOverride : data.RenderNode->GetMaterial();
		BaseShader* pShader = pMaterial->GetShaderVariant();

		auto variantPipeline = _shaderVariantPipelineMap.find(pMaterial->GetVariant());
		if (variantPipeline != _shaderVariantPipelineMap.end())
			ShaderMgr::Get().BuildPipelineSettings((*variantPipeline).second, settings);

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


	template<typename T>
	SceneRenderer::UniformBufferGroup<T>::UniformBufferGroup()
	{
		_current = 0;
	}

	template<typename T>
	bool SceneRenderer::UniformBufferGroup<T>::Init()
	{
		UniformBuffer::CreateInfo uboInfo = {};

		_current = new UniformBufferData();
		uboInfo.isShared = true;
		uboInfo.size = sizeof(T);
		if (!_current->Buffer.Create(uboInfo))
			return false;
		_current->ArrayIndex = 0;
		_buffers.push_back(UniquePtr<UniformBufferData>(_current));
		_data.resize(_current->Buffer.GetMaxSharedUpdates());

		return true;
	}

	template<typename T>
	void SceneRenderer::UniformBufferGroup<T>::Flush()
	{
		_current->Buffer.UpdateShared(_data.data(), _current->UpdateIndex);
	}

	template<typename T>
	void SceneRenderer::UniformBufferGroup<T>::Reset()
	{
		_current = _buffers[0].get();
		_current->UpdateIndex = 0;
	}

	template<typename T>
	void SceneRenderer::UniformBufferGroup<T>::Update(const T& dataBlock, uint& updatedIndex, UniformBufferData** ppUpdatedBuffer, BaseShader* pShader)
	{
		if (_current->UpdateIndex == _data.size())
		{
			//push current udpates to buffer
			_current->Buffer.UpdateShared(_data.data(), _data.size());

			//put this is some generic method if adding more of these types of sharable ubos?
			if (_current->ArrayIndex == _buffers.size() - 1)
			{
				_current = new UniformBufferData();
				UniformBuffer::CreateInfo buffInfo = {};
				buffInfo.isShared = true;
				buffInfo.size = sizeof(T);
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


		_data[_current->UpdateIndex] = dataBlock;

		updatedIndex = _current->UpdateIndex;
		if(ppUpdatedBuffer) *ppUpdatedBuffer = _current;
		_current->UpdateIndex++;

		if (pShader && _current->ShaderBindings.find(pShader) == _current->ShaderBindings.end())
		{
			ShaderBindings::CreateInfo bindingInfo = {};
			bindingInfo.pShader = pShader;
			bindingInfo.type = SBT_OBJECT;
			_current->ShaderBindings[pShader].Create(bindingInfo);
			_current->ShaderBindings[pShader].SetUniformBuffer(ShaderStrings::ObjectBufferName, &_current->Buffer);
		}
	}
}