#include "SceneMgr.h"
#include "Camera.h"
#include "SceneRenderer.h"
#include "imgui.h"
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

		if (!_target.Bind(cmdBuffer))
			return false;

		if (!_renderer->PrepareFrame(GetCameraData()))
			return false;

		if (!_renderer->RenderFrame(cmdBuffer))
			return false;

		if (!_target.Unbind(cmdBuffer))
			return false;

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