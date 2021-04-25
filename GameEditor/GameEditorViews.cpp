#include "SceneMgr.h"
#include "Camera.h"
#include "SceneRenderer.h"
#include "imgui.h"
#include "ResourceMgr.h"
#include "CommandBuffer.h"
#include "ShaderMgr.h"
#include "FilePathMgr.h"
#include "GraphicsWindow.h"
#include "spdlog/spdlog.h"
#include "RenderObject.h"
#include "Animation.h"
#include "GameEditorViews.h"

namespace SunEngine
{
	ICameraView::ICameraView(const String& name) : View(name), _camNode(0)
	{
		_camera = _camNode.AddComponent(new Camera())->As<Camera>();
		_cameraData = _camNode.GetComponentData<CameraComponentData>(_camera);
	}

	ICameraView::~ICameraView()
	{

	}

	void ICameraView::Update(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et)
	{
		View::Update(pWindow, pEvents, nEvents, dt, et);

		Camera* pCamera = GetCamera();
		pCamera->SetFrustum(GetFOV(), GetAspectRatio(), GetNearZ(), GetFarZ());
		GetTransform(_camNode.Position, _camNode.Orientation.Quat);
		_camNode.Orientation.Mode = ORIENT_QUAT;
		_camNode.Update(dt, et);

		if (IsFocused() && IsMouseInside())
		{
			for (uint i = 0; i < nEvents; i++)
			{
				if (pEvents[i].type == GWE_MOUSE_MOVE)
				//if (pEvents[i].type == GWE_MOUSE_DOWN && pEvents[i].mouseButtonCode == MOUSE_MIDDLE)
				{
					glm::vec2 relPos = GetRelativeMousPos() / GetSize();
					relPos.y = 1.0f - relPos.y;
					relPos = relPos * 2.0f - 1.0f;
					glm::vec4 rayDir = glm::vec4(relPos.x, relPos.y, 0.0f, 1.0f);
					rayDir = glm::inverse(pCamera->GetProj()) * rayDir;
					rayDir /= rayDir.w;

					rayDir.w = 0.0f;
					rayDir = _camNode.GetWorld() * rayDir;

					Scene* pScene = SceneMgr::Get().GetActiveScene();

					SceneRayHit hit = {};
					if (pScene->Raycast(glm::vec3(_camNode.GetWorld()[3]), glm::normalize(glm::vec3(rayDir)), hit))
					{
						//spdlog::info("Hit: {}, pos: {},{},{}, norm: {},{},{}", hit.pHitNode->GetNode()->GetName(), hit.position.x, hit.position.y, hit.position.z, hit.normal.x, hit.normal.y, hit.normal.z);
					}

					break;
				}
			}
		}
	}

	SceneView::SceneView(SceneRenderer* pRenderer) : ICameraView("SceneView")
	{
		_renderer = pRenderer;
	}

	SceneView::~SceneView()
	{

	}

	bool SceneView::Render(CommandBuffer* cmdBuffer)
	{
		if (!_renderer)
			return false;

		if (!_renderer->PrepareFrame(GetCameraData()))
			return false;

		RenderTargetPassInfo outputInfo = {};
		outputInfo.pTarget = &_outputTarget;
		outputInfo.pPipeline = &_outputData.first;
		outputInfo.pBindings = &_outputData.second;

		bool bDeferred = EngineInfo::GetRenderer().RenderMode() == EngineInfo::Renderer::Deferred;
		DeferredRenderTargetPassInfo deferredInfo = {};
		if (bDeferred)
		{
			deferredInfo.pTarget = &_deferredTarget;
			deferredInfo.pPipeline = &_deferredData.first;
			deferredInfo.pBindings = &_deferredData.second;

			deferredInfo.pDeferredResolveTarget = &_deferredResolveTarget;
			deferredInfo.pDeferredCopyPipeline = &_deferredCopyData.first;
			deferredInfo.pDeferredCopyBindings = &_deferredCopyData.second;

#ifdef SUPPORT_SSR
			deferredInfo.pSSRPipeline = &_ssrData.first;
			deferredInfo.pSSRBindings = &_ssrData.second;
#endif
		}

		if (!_renderer->RenderFrame(cmdBuffer, &_opaqueTarget, &outputInfo, bDeferred ? &deferredInfo : 0))
			return false;

		//Apply gamma correction
		_target.Bind(cmdBuffer);

		if (!_gammaData.first.GetShader()->Bind(cmdBuffer))
			return false;

		if (!_gammaData.first.Bind(cmdBuffer))
			return false;

		if (!_gammaData.second.Bind(cmdBuffer))
			return false;

		cmdBuffer->Draw(6, 1, 0, 0);

		if (!_gammaData.second.Unbind(cmdBuffer))
			return false;

		if (!_gammaData.first.Unbind(cmdBuffer))
			return false;

		if (!_gammaData.first.GetShader()->Unbind(cmdBuffer))
			return false;


		_target.Unbind(cmdBuffer);

		return true;
	}

	void SceneView::RenderGUI()
	{
		Scene* pScene = SceneMgr::Get().GetActiveScene();

		ImGui::TableNextColumn();
		BuildSceneTree(pScene->GetRoot());

		ImGui::TableNextColumn();
		BuildSelectedNodeGUI(pScene);
	}

	bool SceneView::OnCreate(const CreateInfo& info)
	{
		if (!CreateRenderPassData(DefaultShaders::SceneCopy, _outputData))
			return false;

		if (!CreateRenderPassData(DefaultShaders::Gamma, _gammaData))
			return false;

		//TODO lots of duplicate code basically
		bool bDeferred = EngineInfo::GetRenderer().RenderMode() == EngineInfo::Renderer::Deferred;
		if (bDeferred)
		{
			if (!CreateRenderPassData(DefaultShaders::Deferred, _deferredData))
				return false;

			if (!CreateRenderPassData(DefaultShaders::SceneCopy, _deferredCopyData))
				return false;

#ifdef SUPPORT_SSR
			if (!CreateRenderPassData(DefaultShaders::ScreenSpaceReflection, _ssrData))
				return false;
#endif
		}

		return OnResize(info);
	}

	bool SceneView::OnResize(const CreateInfo& info)
	{
		RenderTarget::CreateInfo sceneInfo = {};
		sceneInfo.width = info.width;
		sceneInfo.height = info.height;
		sceneInfo.numTargets = 1;
		sceneInfo.hasDepthBuffer = true;
		sceneInfo.floatingPointColorBuffer = true;

		if (!_outputTarget.Create(sceneInfo))
			return false;

		if (!_gammaData.second.SetTexture(MaterialStrings::DiffuseMap, _outputTarget.GetColorTexture()))
			return false;

		if (!_opaqueTarget.Create(sceneInfo))
			return false;

		_outputData.second.SetTexture(MaterialStrings::DiffuseMap, _opaqueTarget.GetColorTexture());
		_outputData.second.SetTexture(MaterialStrings::DepthMap, _opaqueTarget.GetDepthTexture());

		if (EngineInfo::GetRenderer().RenderMode() == EngineInfo::Renderer::Deferred)
		{
			sceneInfo.hasDepthBuffer = false;
			if (!_deferredResolveTarget.Create(sceneInfo))
				return false;

			sceneInfo.hasDepthBuffer = true;
			sceneInfo.numTargets = 4;
			if (!_deferredTarget.Create(sceneInfo))
				return false;

			_deferredData.second.SetTexture(MaterialStrings::DiffuseMap, _deferredTarget.GetColorTexture(0));
			_deferredData.second.SetTexture(MaterialStrings::SpecularMap, _deferredTarget.GetColorTexture(1));
			_deferredData.second.SetTexture(MaterialStrings::NormalMap, _deferredTarget.GetColorTexture(2));
			_deferredData.second.SetTexture(MaterialStrings::PositionMap, _deferredTarget.GetColorTexture(3));
			_deferredData.second.SetTexture(MaterialStrings::DepthMap, _deferredTarget.GetDepthTexture());

			_deferredCopyData.second.SetTexture(MaterialStrings::DiffuseMap, _deferredResolveTarget.GetColorTexture(0)); //Feed in previous frame results
			_deferredCopyData.second.SetTexture(MaterialStrings::DepthMap, _deferredTarget.GetDepthTexture()); //used for blend factor blend

#ifdef SUPPORT_SSR
			_ssrData.second.SetTexture(MaterialStrings::DiffuseMap, _deferredResolveTarget.GetColorTexture(0)); //Feed in previous frame results
			_ssrData.second.SetTexture(MaterialStrings::SpecularMap, _deferredTarget.GetColorTexture(1)); //used for blend factor blend
			_ssrData.second.SetTexture(MaterialStrings::NormalMap, _deferredTarget.GetColorTexture(2));
			_ssrData.second.SetTexture(MaterialStrings::PositionMap, _deferredTarget.GetColorTexture(3));
#endif
		}

		return true;
	}

	void SceneView::BuildSceneTree(SceneNode* pNode)
	{
		if(ImGui::TreeNode(pNode->GetName().c_str()))
		{
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				_selNodeName = pNode->GetName();
			}

			for (auto iter = pNode->GetChildIterator(); !iter.End(); ++iter)
			{
				BuildSceneTree(*iter);
			}
			ImGui::TreePop();
		}
	}

	void SceneView::BuildSelectedNodeGUI(Scene* pScene)
	{
		bool removeNode = false;

		ImGui::BeginChild("SelNodeEditor");
		SceneNode* pNode = pScene->GetNode(_selNodeName);
		if (pNode)
		{
			ImGui::Text(pNode->GetName().c_str());
			if (ImGui::Button("Remove"))
			{
				removeNode = true;
			}
			ImGui::DragFloat3("Position", &pNode->Position[0]);
			ImGui::DragFloat3("Scale", &pNode->Scale[0]);
			ImGui::DragFloat3("Angles", &pNode->Orientation.Angles[0]);

			for (auto iter = pNode->BeginComponent(); iter != pNode->EndComponent(); ++iter)
			{
				Component* c = *iter;
				if (c->GetType() == COMPONENT_ANIMATOR)
				{
					AnimatorComponentData* animData = pNode->GetComponentData<AnimatorComponentData>(c);
					if (ImGui::TreeNode("Animator"))
					{
						bool playing = animData->GetPlaying();
						if (ImGui::Checkbox("Playing", &playing)) animData->SetPlaying(playing);

						ImGui::TreePop();
					}
				}
			}
		}
		else
		{
			_selNodeName.clear();
		}

		ImGui::EndChild();

		if (removeNode)
		{
			pScene->RemoveNode(pNode);
		}
	}

	bool SceneView::CreateRenderPassData(const String& shader, Pair<GraphicsPipeline, ShaderBindings>& data)
	{
		BaseShader* pShader = ShaderMgr::Get().GetShader(shader)->GetDefault();
		assert(pShader);

		if (!pShader)
			return false;

		GraphicsPipeline::CreateInfo pipelineInfo = {};
		pipelineInfo.pShader = pShader;

		//TODO: specific pipelineInfo based on shader string name
		if (shader == DefaultShaders::ScreenSpaceReflection)
		{
			pipelineInfo.settings.EnableAlphaBlend();
			pipelineInfo.settings.blendState.srcAlphaBlendFactor = SE_BF_ONE;
			pipelineInfo.settings.blendState.dstAlphaBlendFactor = SE_BF_ZERO;
			//pipelineInfo.settings.blendState.srcColorBlendFactor = SE_BF_SRC_ALPHA;
			//pipelineInfo.settings.blendState.dstColorBlendFactor = SE_BF_ONE_MINUS_SRC_ALPHA;
			//pipelineInfo.settings.depthStencil.depthCompareOp = se; //we only want to test pixels which have depths < 1, the quad will have depth == 1, so pixels which are the background wont be tested
		}

		if (!data.first.Create(pipelineInfo))
			return false;

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.pShader = pShader;
		bindingInfo.type = SBT_MATERIAL;

		if (!data.second.Create(bindingInfo))
			return false;

		data.second.SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(SE_FM_NEAREST, SE_WM_CLAMP_TO_EDGE, SE_AM_OFF));

		return true;
	}

}