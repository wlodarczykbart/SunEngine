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
#include "SceneRenderer.h"

namespace SunEngine
{
	SceneRenderer::SceneRenderer()
	{
		_bInit = false;
		_currentCamera = 0;
		_currentSunlight = 0;
		_currentObjectBuffer = 0;

		_cameraBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
		_lightBuffer = UniquePtr<UniformBufferData>(new UniformBufferData());
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	bool SceneRenderer::Init()
	{
		UniformBuffer::CreateInfo uboInfo = {};
		uboInfo.isShared = false;

		uboInfo.size = sizeof(CameraBufferData);
		if (!_cameraBuffer->Buffer.Create(uboInfo))
			return false;

		uboInfo.size = sizeof(SunlightBufferData);
		if (!_lightBuffer->Buffer.Create(uboInfo))
			return false;

		_currentObjectBuffer = new UniformBufferData();
		uboInfo.isShared = true;
		uboInfo.size = sizeof(ObjectBufferData);
		if (!_currentObjectBuffer->Buffer.Create(uboInfo))
			return false;
		_currentObjectBuffer->ArrayIndex = 0;
		_objectBuffers.push_back(UniquePtr<UniformBufferData>(_currentObjectBuffer));
		_objectBufferData.resize(_currentObjectBuffer->Buffer.GetMaxSharedUpdates());

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
		_currentObjectBuffer->Buffer.UpdateShared(_objectBufferData.data(), _currentObjectBuffer->UpdateIndex);

		if (!_currentCamera)
			return false;

		if (!_currentSunlight)
			return false;

		//reset to start at this point 
		_currentObjectBuffer = _objectBuffers[0].get();
		return true;
	}

	bool SceneRenderer::RenderFrame(CommandBuffer* cmdBuffer, RenderTarget* pOpaqueTarget, RenderTargetPassInfo* pOutputInfo, DeferredRenderTargetPassInfo* pDeferredInfo)
	{
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
		if (!_cameraBuffer->Buffer.Update(&camData))
			return false;

		SunlightBufferData sunData = {};
		glm::vec4 sunDirView = view * _currentSunlight->Direction;
		sunData.Color.Set(&_currentSunlight->C()->As<Light>()->GetColor());
		sunData.Direction.Set(&_currentSunlight->Direction);
		sunData.ViewDirection.Set(&sunDirView);
		if (!_lightBuffer->Buffer.Update(&sunData))
			return false;

		//plug in deferred shader so it gets camera/light buffers
		if (pDeferredInfo)
		{
			_currentShaders.insert(pDeferredInfo->pPipeline->GetShader());
			_currentShaders.insert(pDeferredInfo->pSSRPipeline->GetShader());
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
		}

		BaseShader* pShader = 0;

		if (pDeferredInfo)
		{
			pDeferredInfo->pTarget->Bind(cmdBuffer);
			ProcessRenderQueue(cmdBuffer, _deferredRenderQueue);
			pDeferredInfo->pTarget->Unbind(cmdBuffer);

			pDeferredInfo->pDeferredResolveTarget->SetClearColor(1, 0, 0, 1);
			//Draw deferred objects to the deferred resolve render target
			pDeferredInfo->pDeferredResolveTarget->Bind(cmdBuffer);

			pShader = pDeferredInfo->pPipeline->GetShader();
			pShader->Bind(cmdBuffer);
			pDeferredInfo->pPipeline->Bind(cmdBuffer);
			pDeferredInfo->pBindings->Bind(cmdBuffer);

			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer.get());
			TryBindBuffer(cmdBuffer, pShader, _lightBuffer.get());

			//draw 2 triangles that are generated by screen vertex shader
			cmdBuffer->Draw(6, 1, 0, 0);

			pDeferredInfo->pBindings->Unbind(cmdBuffer);
			pDeferredInfo->pPipeline->Unbind(cmdBuffer);
			pShader->Unbind(cmdBuffer);

			pDeferredInfo->pDeferredResolveTarget->Unbind(cmdBuffer);

			pOpaqueTarget->SetClearColor(0, 1, 0, 1);
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

				TryBindBuffer(cmdBuffer, pShader, _cameraBuffer.get());

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

		ProcessRenderQueue(cmdBuffer, _opaqueRenderQueue);

		pOpaqueTarget->Unbind(cmdBuffer);

		pOutputInfo->pTarget->SetClearColor(0, 0, 1, 1);
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

		ProcessRenderQueue(cmdBuffer, _sortedRenderQueue);

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
					bool isValid = renderNode.GetMaterial() && renderNode.GetMaterial()->GetShader() && renderNode.GetMesh();

					if (isValid)
					{
						RenderNodeData data = {};
						data.SceneNode = pNode;
						data.RenderNode = &renderNode;
						data.Pipeline = GetPipeline(renderNode);

						if (_currentObjectBuffer->UpdateIndex == _objectBufferData.size())
						{
							//push current udpates to buffer
							_currentObjectBuffer->Buffer.UpdateShared(_objectBufferData.data(), _objectBufferData.size());

							//put this is some generic method if adding more of these types of sharable ubos?
							if (_currentObjectBuffer->ArrayIndex == _objectBuffers.size() - 1)
							{
								_currentObjectBuffer = new UniformBufferData();
								UniformBuffer::CreateInfo buffInfo = {};
								buffInfo.isShared = true;
								buffInfo.size = sizeof(ObjectBufferData);
								if (!_currentObjectBuffer->Buffer.Create(buffInfo))
									return;

								_currentObjectBuffer->ArrayIndex = _objectBuffers.size() - 1;
								_objectBuffers.push_back(UniquePtr<UniformBufferData>(_currentObjectBuffer));
							}
							else
							{
								_currentObjectBuffer = _objectBuffers[_currentObjectBuffer->ArrayIndex + 1].get();
							}

							_currentObjectBuffer->UpdateIndex = 0;
						}

						ObjectBufferData objBuffer = {};
						glm::mat4 itp = glm::transpose(glm::inverse(renderNode.GetWorld()));
						objBuffer.WorldMatrix.Set(&renderNode.GetWorld());
						objBuffer.InverseTransposeMatrix.Set(&itp);
						_objectBufferData[_currentObjectBuffer->UpdateIndex] = objBuffer;

						data.ObjectBufferIndex = _currentObjectBuffer->UpdateIndex;
						data.ObjectBindings = _currentObjectBuffer;
						_currentObjectBuffer->UpdateIndex++;

						BaseShader* pShader = renderNode.GetMaterial()->GetShader()->GetVariant(renderNode.GetMaterial()->GetVariant());
						if (data.ObjectBindings->ShaderBindings.find(pShader) == data.ObjectBindings->ShaderBindings.end())
						{
							ShaderBindings::CreateInfo bindingInfo = {};
							bindingInfo.pShader = pShader;
							bindingInfo.type = SBT_OBJECT;
							data.ObjectBindings->ShaderBindings[pShader].Create(bindingInfo);
							data.ObjectBindings->ShaderBindings[pShader].SetUniformBuffer(ShaderStrings::ObjectBufferName, &data.ObjectBindings->Buffer);
						}

						_currentShaders.insert(pShader);

						if (data.RenderNode->GetMaterial()->GetVariant() == Shader::Default)
						{
							//TODO: add logic to place these in either opaque or sorted based on the render node properties
							_sortedRenderQueue.push(data);
						}
						else if (data.RenderNode->GetMaterial()->GetVariant() == Shader::Deferred)
						{
							_deferredRenderQueue.push(data);
						}
					}
				}
			}
		}
	}

	void SceneRenderer::ProcessRenderQueue(CommandBuffer* cmdBuffer, Queue<RenderNodeData>& queue)
	{
		IShaderBindingsBindState objectBindData = {};
		objectBindData.DynamicIndices[0].first = ShaderStrings::ObjectBufferName;

		while (queue.size())
		{
			RenderNodeData& renderData = queue.front();

			BaseShader* pShader = renderData.RenderNode->GetMaterial()->GetShader()->GetVariant(renderData.RenderNode->GetMaterial()->GetVariant());
			GraphicsPipeline& pipeline = *renderData.Pipeline;

			//pipeline.GetShader()->getgpu
			pShader->Bind(cmdBuffer);
			pipeline.Bind(cmdBuffer);
			TryBindBuffer(cmdBuffer, pShader, _cameraBuffer.get());
			TryBindBuffer(cmdBuffer, pShader, _lightBuffer.get());

			renderData.RenderNode->GetMesh()->GetGPUObject()->Bind(cmdBuffer);
			renderData.RenderNode->GetMaterial()->GetGPUObject()->Bind(cmdBuffer);

			objectBindData.DynamicIndices[0].second = renderData.ObjectBufferIndex;
			renderData.ObjectBindings->ShaderBindings.at(pShader).Bind(cmdBuffer, &objectBindData);

			cmdBuffer->DrawIndexed(
				renderData.RenderNode->GetIndexCount(),
				renderData.RenderNode->GetInstanceCount(),
				renderData.RenderNode->GetFirstIndex(),
				renderData.RenderNode->GetVertexOffset(),
				0);

			queue.pop();
		}
	}

	GraphicsPipeline* SceneRenderer::GetPipeline(const RenderNode& node)
	{
		PipelineSettings settings = {};
		node.BuildPipelineSettings(settings);
		BaseShader* pShader = node.GetMaterial()->GetShader()->GetVariant(node.GetMaterial()->GetVariant());

		for (uint i = 0; i < _graphicsPipelines.size(); i++)
		{
			if (_graphicsPipelines[i]->GetShader() == pShader && settings == _graphicsPipelines[i]->GetSettings())
			{
				return _graphicsPipelines[i].get();
			}
		}

		GraphicsPipeline* pipeline = new GraphicsPipeline();

		GraphicsPipeline::CreateInfo info = {};
		info.pShader = pShader;
		info.settings = settings;

		if (!pipeline->Create(info))
			return 0;

		_graphicsPipelines.push_back(UniquePtr<GraphicsPipeline>(pipeline));
		return _graphicsPipelines.back().get();
	}

	void SceneRenderer::TryBindBuffer(CommandBuffer* cmdBuffer, BaseShader* pShader, UniformBufferData* buffer) const
	{
		auto found = buffer->ShaderBindings.find(pShader);
		if(found != buffer->ShaderBindings.end())
			(*found).second.Bind(cmdBuffer);
	}

}