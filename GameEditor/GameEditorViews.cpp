#include "SceneMgr.h"
#include "Camera.h"
#include "SceneRenderer.h"
#include "imgui.h"
#include "ResourceMgr.h"
#include "CommandBuffer.h"
#include "GameEditorViews.h"

namespace SunEngine
{
	ICameraView::ICameraView(const String& name) : View(name)
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

		//Render the scene
		if (!_sceneTarget.Bind(cmdBuffer))
			return false;

		if (!_renderer->PrepareFrame(GetCameraData()))
			return false;

		if (!_renderer->RenderFrame(cmdBuffer))
			return false;

		if (!_sceneTarget.Unbind(cmdBuffer))
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
		Shader* pShader = ResourceMgr::Get().GetShader(DefaultResource::Shader::Gamma);
		assert(pShader);

		if (!pShader)
			return false;

		GraphicsPipeline::CreateInfo pipelineInfo = {};
		pipelineInfo.pShader = pShader->GetGPUObject();
		if (!_gammaData.first.Create(pipelineInfo))
			return false;

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.pShader = pShader->GetGPUObject();
		bindingInfo.type = SBT_MATERIAL;

		if (!_gammaData.second.Create(bindingInfo))
			return false;

		_gammaData.second.SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE, SE_AM_OFF));
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

		if (!_sceneTarget.Create(sceneInfo))
			return false;

		if (!_gammaData.second.SetTexture(MaterialStrings::DiffuseMap, _sceneTarget.GetColorTexture()))
			return false;

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

}