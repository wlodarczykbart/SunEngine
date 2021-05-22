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
#include "MeshRenderer.h"
#include "ResourceMgr.h"
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
		pCamera->SetProjection(GetFOV(), GetAspectRatio(), GetNearZ(), GetFarZ());
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
					//spdlog::info("{} {}", relPos.x, relPos.y);
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
		_resizeOccured = false;
		_renderer = pRenderer;
		_shaderBufferUsageCount = 0;
		_debugFrustum = 0;
	}

	SceneView::~SceneView()
	{

	}

	bool SceneView::Render(CommandBuffer* cmdBuffer)
	{
		if (!_renderer)
			return false;

		static glm::mat4 bufferData[64];
		glm::vec4 invScreenSize = glm::vec4(1.0f / GetRenderTarget()->Width(), 1.0f / GetRenderTarget()->Height(), 0.0f, 0.0f);

		glm::mat4 fxaaData;
		fxaaData[0] = invScreenSize;
		fxaaData[1] = glm::vec4(_settings.fxaa.subpixel, _settings.fxaa.edgeThreshold, _settings.fxaa.edgeThresholdMin, _settings.fxaa.enabled ? 1.0f : 0.0f);
		bufferData[_fxaaData.bufferIndex] = fxaaData;
		IShaderBindingsBindState bufferBindState = {};
		bufferBindState.DynamicIndices[0].first = ShaderStrings::MaterialBufferName;

		_shaderBuffer.UpdateShared(bufferData, _shaderBufferUsageCount);

		if (!_renderer->PrepareFrame(_outputTarget.GetColorTexture(), _resizeOccured, GetCameraData()))
			return false;
		_resizeOccured = false;

		RenderTargetPassInfo outputInfo = {};
		outputInfo.pTarget = &_outputTarget;
		outputInfo.pPipeline = &_outputData.pipeline;
		outputInfo.pBindings = &_outputData.bindings;

		bool bDeferred = EngineInfo::GetRenderer().RenderMode() == EngineInfo::Renderer::Deferred;
		DeferredRenderTargetPassInfo deferredInfo = {};
		if (bDeferred)
		{
			deferredInfo.pTarget = &_deferredTarget;
			deferredInfo.pPipeline = &_deferredData.pipeline;
			deferredInfo.pBindings = &_deferredData.bindings;

			deferredInfo.pDeferredResolveTarget = &_deferredResolveTarget;
			deferredInfo.pDeferredCopyPipeline = &_deferredCopyData.pipeline;
			deferredInfo.pDeferredCopyBindings = &_deferredCopyData.bindings;

#ifdef SUPPORT_SSR
			deferredInfo.pSSRPipeline = &_ssrData.first;
			deferredInfo.pSSRBindings = &_ssrData.second;
#endif
		}

		if (!_renderer->RenderFrame(cmdBuffer, &_opaqueTarget, &outputInfo, bDeferred ? &deferredInfo : 0))
			return false;

		//Apply toneMap correction
		_toneMapTarget.Bind(cmdBuffer);

		if (!_toneMapData.pipeline.GetShader()->Bind(cmdBuffer)) return false;
		if (!_toneMapData.pipeline.Bind(cmdBuffer)) return false;
		if (!_toneMapData.bindings.Bind(cmdBuffer)) return false;
		cmdBuffer->Draw(6, 1, 0, 0);
		if (!_toneMapData.bindings.Unbind(cmdBuffer)) return false;
		if (!_toneMapData.pipeline.Unbind(cmdBuffer)) return false;
		if (!_toneMapData.pipeline.GetShader()->Unbind(cmdBuffer)) return false;

		_toneMapTarget.Unbind(cmdBuffer);

		_target.Bind(cmdBuffer);

		bufferBindState.DynamicIndices[0].second = _fxaaData.bufferIndex;
		if (!_fxaaData.pipeline.GetShader()->Bind(cmdBuffer)) return false;
		if (!_fxaaData.pipeline.Bind(cmdBuffer)) return false;
		if (!_fxaaData.bindings.Bind(cmdBuffer, &bufferBindState)) return false;
		cmdBuffer->Draw(6, 1, 0, 0);
		if (!_fxaaData.bindings.Unbind(cmdBuffer)) return false;
		if (!_fxaaData.pipeline.Unbind(cmdBuffer)) return false;
		if (!_fxaaData.pipeline.GetShader()->Unbind(cmdBuffer)) return false;

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

#define LOG_CORNER(c) spdlog::info("{} {} {} {}", #c, corners[c].x, corners[c].y, corners[c].z)

	void SceneView::Update(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et)
	{
		ICameraView::Update(pWindow, pEvents, nEvents, dt, et);

#if 0
		if (_debugFrustum == 0)
		{
			Scene* pScene = SceneMgr::Get().GetActiveScene();

			_debugFrustum = pScene->AddNode("DebugFrustum");
			MeshRenderer* pMeshRenderer = _debugFrustum->AddComponent(new MeshRenderer())->As<MeshRenderer>();

			Mesh* pMesh = ResourceMgr::Get().AddMesh("DebugFrustumMesh");
			pMesh->AllocateCube();
			pMesh->UpdateBoundingVolume();
			pMesh->RegisterToGPU();
			pMeshRenderer->SetMesh(pMesh);
			pMeshRenderer->SetMaterial(ResourceMgr::Get().GetMaterial(DefaultResource::Material::StandardSpecular));
			_debugFrustum->Initialize();
		}

		for(uint i = 0; i < nEvents; i++)
		{
			if (pEvents[i].type == GWE_KEY_DOWN && pEvents[i].keyCode == KEY_F)
			{

				Mesh* pMesh = _debugFrustum->GetComponentOfType(COMPONENT_MESH_RENDERER)->As<MeshRenderer>()->GetMesh();
				glm::vec3* corners = GetCameraData()->FrustumCorners;

				LOG_CORNER(CameraComponentData::LBN);
				LOG_CORNER(CameraComponentData::RBN);
				LOG_CORNER(CameraComponentData::RTN);
				LOG_CORNER(CameraComponentData::LTN);
				LOG_CORNER(CameraComponentData::LBF);
				LOG_CORNER(CameraComponentData::RBF);
				LOG_CORNER(CameraComponentData::RTF);
				LOG_CORNER(CameraComponentData::LTF);

				pMesh->SetVertexVar(0, glm::vec4(corners[CameraComponentData::LBN], 1.0f));
				pMesh->SetVertexVar(1, glm::vec4(corners[CameraComponentData::RBN], 1.0f));
				pMesh->SetVertexVar(2, glm::vec4(corners[CameraComponentData::RTN], 1.0f));
				pMesh->SetVertexVar(3, glm::vec4(corners[CameraComponentData::LTN], 1.0f));

				pMesh->SetVertexVar(4, glm::vec4(corners[CameraComponentData::RBN], 1.0f));
				pMesh->SetVertexVar(5, glm::vec4(corners[CameraComponentData::RBF], 1.0f));
				pMesh->SetVertexVar(6, glm::vec4(corners[CameraComponentData::RTF], 1.0f));
				pMesh->SetVertexVar(7, glm::vec4(corners[CameraComponentData::RTN], 1.0f));

				pMesh->SetVertexVar(8, glm::vec4(corners[CameraComponentData::LBF], 1.0f));
				pMesh->SetVertexVar(9, glm::vec4(corners[CameraComponentData::LBN], 1.0f));
				pMesh->SetVertexVar(10, glm::vec4(corners[CameraComponentData::LTN], 1.0f));
				pMesh->SetVertexVar(11, glm::vec4(corners[CameraComponentData::LTF], 1.0f));

				pMesh->SetVertexVar(12, glm::vec4(corners[CameraComponentData::RBF], 1.0f));
				pMesh->SetVertexVar(13, glm::vec4(corners[CameraComponentData::LBF], 1.0f));
				pMesh->SetVertexVar(14, glm::vec4(corners[CameraComponentData::LTF], 1.0f));
				pMesh->SetVertexVar(15, glm::vec4(corners[CameraComponentData::RTF], 1.0f));

				pMesh->SetVertexVar(16, glm::vec4(corners[CameraComponentData::LTN], 1.0f));
				pMesh->SetVertexVar(17, glm::vec4(corners[CameraComponentData::RTN], 1.0f));
				pMesh->SetVertexVar(18, glm::vec4(corners[CameraComponentData::RTF], 1.0f));
				pMesh->SetVertexVar(19, glm::vec4(corners[CameraComponentData::LTF], 1.0f));

				pMesh->SetVertexVar(20, glm::vec4(corners[CameraComponentData::LBF], 1.0f));
				pMesh->SetVertexVar(21, glm::vec4(corners[CameraComponentData::RBF], 1.0f));
				pMesh->SetVertexVar(22, glm::vec4(corners[CameraComponentData::RBN], 1.0f));
				pMesh->SetVertexVar(23, glm::vec4(corners[CameraComponentData::LBN], 1.0f));

				pMesh->RegisterToGPU();
				break;
			}
		}
#endif

	}

	bool SceneView::OnCreate(const CreateInfo& info)
	{
		//shader uniform buffer for post process rendering, at the moment each shader has at most 16 floats of input
		UniformBuffer::CreateInfo buffInfo = {};
		buffInfo.isShared = true;
		buffInfo.size = sizeof(glm::mat4);
		if (!_shaderBuffer.Create(buffInfo))
			return false;

		if (!CreateRenderPassData(DefaultShaders::SceneCopy, _outputData))
			return false;

		if (!CreateRenderPassData(DefaultShaders::ToneMap, _toneMapData))
			return false;

		if (!CreateRenderPassData(DefaultShaders::FXAA, _fxaaData))
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
		RenderTarget::CreateInfo rtInfo = {};
		rtInfo.width = info.width;
		rtInfo.height = info.height;
		rtInfo.numTargets = 1;
		rtInfo.hasDepthBuffer = true;
		rtInfo.floatingPointColorBuffer = true;

		if (!_outputTarget.Create(rtInfo))
			return false;

		if (!_toneMapData.bindings.SetTexture(MaterialStrings::DiffuseMap, _outputTarget.GetColorTexture()))
			return false;

		if (!_opaqueTarget.Create(rtInfo))
			return false;

		if (!_outputData.bindings.SetTexture(MaterialStrings::DiffuseMap, _opaqueTarget.GetColorTexture()))
			return false;

		if (!_outputData.bindings.SetTexture(MaterialStrings::DepthMap, _opaqueTarget.GetDepthTexture()))
			return false;

		if (EngineInfo::GetRenderer().RenderMode() == EngineInfo::Renderer::Deferred)
		{
			rtInfo.hasDepthBuffer = false;
			if (!_deferredResolveTarget.Create(rtInfo))
				return false;

			rtInfo.hasDepthBuffer = true;
			rtInfo.numTargets = 4;
			if (!_deferredTarget.Create(rtInfo))
				return false;

			_deferredData.bindings.SetTexture(MaterialStrings::DiffuseMap, _deferredTarget.GetColorTexture(0));
			_deferredData.bindings.SetTexture(MaterialStrings::SpecularMap, _deferredTarget.GetColorTexture(1));
			_deferredData.bindings.SetTexture(MaterialStrings::NormalMap, _deferredTarget.GetColorTexture(2));
			_deferredData.bindings.SetTexture(MaterialStrings::PositionMap, _deferredTarget.GetColorTexture(3));
			_deferredData.bindings.SetTexture(MaterialStrings::DepthMap, _deferredTarget.GetDepthTexture());

			_deferredCopyData.bindings.SetTexture(MaterialStrings::DiffuseMap, _deferredResolveTarget.GetColorTexture(0)); //Feed in previous frame results
			_deferredCopyData.bindings.SetTexture(MaterialStrings::DepthMap, _deferredTarget.GetDepthTexture()); //used for blend factor blend

#ifdef SUPPORT_SSR
			_ssrData.second.SetTexture(MaterialStrings::DiffuseMap, _deferredResolveTarget.GetColorTexture(0)); //Feed in previous frame results
			_ssrData.second.SetTexture(MaterialStrings::SpecularMap, _deferredTarget.GetColorTexture(1)); //used for blend factor blend
			_ssrData.second.SetTexture(MaterialStrings::NormalMap, _deferredTarget.GetColorTexture(2));
			_ssrData.second.SetTexture(MaterialStrings::PositionMap, _deferredTarget.GetColorTexture(3));
#endif
		}

		rtInfo.numTargets = 1;
		rtInfo.hasDepthBuffer = false;
		rtInfo.floatingPointColorBuffer = false;
		if (!_toneMapTarget.Create(rtInfo))
			return false;

		if (!_fxaaData.bindings.SetTexture(MaterialStrings::DiffuseMap, _toneMapTarget.GetColorTexture()))
			return false;

		_resizeOccured = true;

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
					Animator* animator = c->As<Animator>();
					AnimatorComponentData* animData = pNode->GetComponentData<AnimatorComponentData>(c);
					if (ImGui::TreeNode("Animator"))
					{
						bool playing = animData->GetPlaying();
						bool loop = animData->GetLoop();
						float speed = animData->GetSpeed();
						int clip = (int)animData->GetClip();

						if (ImGui::Checkbox("Playing", &playing)) animData->SetPlaying(playing);
						if (ImGui::Checkbox("Loop", &loop)) animData->SetLoop(loop);
						if (ImGui::DragInt("Clip", &clip, 0.1f, 0, animator->GetClipCount())) animData->SetClip((uint)clip);
						if (ImGui::DragFloat("Speed", &speed, 0.05f, 0.0f, 10.0f)) animData->SetSpeed(speed);

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

	bool SceneView::CreateRenderPassData(const String& shader, RenderPassData& data, bool useOneZ)
	{
		BaseShader* pShader = ShaderMgr::Get().GetShader(shader)->GetVariant(useOneZ ? Shader::OneZ : Shader::Default);
		assert(pShader);

		if (!pShader)
			return false;

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.pShader = pShader;
		bindingInfo.type = SBT_MATERIAL;

		if (!data.bindings.Create(bindingInfo))
			return false;

		data.bindings.SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE));

		if (data.bindings.SetUniformBuffer(ShaderStrings::MaterialBufferName, &_shaderBuffer))
			data.bufferIndex = _shaderBufferUsageCount++;
		else
			data.bufferIndex = -1;

		GraphicsPipeline::CreateInfo pipelineInfo = {};

		//TODO: make sure this in fact should be default for screen space pipelines
		pipelineInfo.settings.depthStencil.depthCompareOp = SE_DC_LESS_EQUAL;
		pipelineInfo.settings.depthStencil.enableDepthWrite = false;

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
		else if (shader == DefaultShaders::SceneCopy)
		{
			//this pipeline writes to depth buffer in pixel shader
			pipelineInfo.settings.depthStencil.enableDepthWrite = true;
		}

		pipelineInfo.pShader = pShader;
		if (!data.pipeline.Create(pipelineInfo))
			return false;

		return true;
	}

	SceneView::Settings::Settings()
	{
		fxaa.enabled = true;
		fxaa.subpixel = 0.75f;
		fxaa.edgeThreshold = 0.166f;
		fxaa.edgeThresholdMin = 0.0833f;

		memset(&gui, 0x0, sizeof(gui));
	}

}