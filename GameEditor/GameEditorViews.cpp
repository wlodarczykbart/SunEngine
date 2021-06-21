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
#include "GameEditorGUI.h"
#include "StringUtil.h"
#include "Terrain.h"
#include "Environment.h"
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
			deferredInfo.pSSRPipeline = &_ssrData.pipeline;
			deferredInfo.pSSRBindings = &_ssrData.bindings;
#endif
		}

		RenderTargetPassInfo msaaResolvInfo = {};
		msaaResolvInfo.pTarget = &_msaaTarget;
		msaaResolvInfo.pPipeline = &_msaaResolveData.pipeline;
		msaaResolvInfo.pBindings = &_msaaResolveData.bindings;
		bool bMSAA = _settings.msaa.enabled && EngineInfo::GetRenderer().GetMSAAMode() != SE_MSAA_OFF;

		if (!_renderer->RenderFrame(cmdBuffer, &_opaqueTarget, &outputInfo, bDeferred ? &deferredInfo : 0, bMSAA ? &msaaResolvInfo : 0))
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

	void SceneView::RenderGUI(GUIRenderer* pRenderer)
	{
		Scene* pScene = SceneMgr::Get().GetActiveScene();

		ImGui::TableNextColumn();
		BuildSceneTree(pScene->GetRoot());

		ImGui::TableNextColumn();
		BuildSelectedNodeGUI(pScene, pRenderer);
	}

#define LOG_CORNER(c) spdlog::info("{} {} {} {}", #c, corners[c].x, corners[c].y, corners[c].z)

	void SceneView::Update(GraphicsWindow* pWindow, const GWEventData* pEvents, uint nEvents, float dt, float et)
	{
		ICameraView::Update(pWindow, pEvents, nEvents, dt, et);
		SetVisible(true);

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

		if (IsMouseInside())
		{
			for (uint i = 0; i < nEvents; i++)
			{
				//if (pEvents[i].type == GWE_MOUSE_MOVE)
				if (pEvents[i].type == GWE_MOUSE_DOWN && pEvents[i].mouseButtonCode == MOUSE_MIDDLE)
				{
					auto pCamData = GetCameraData();
					
					glm::mat4 invView = glm::inverse(pCamData->GetView());
					glm::mat4 invProj = pCamData->C()->As<Camera>()->GetInvProj();

					glm::vec2 relPos = GetRelativeMousPos() / GetSize();
					//spdlog::info("{} {}", relPos.x, relPos.y);
					relPos.y = 1.0f - relPos.y;
					relPos = relPos * 2.0f - 1.0f;
					glm::vec4 rayDir = glm::vec4(relPos.x, relPos.y, 0.0f, 1.0f);
					rayDir = invProj * rayDir;
					rayDir /= rayDir.w;

					rayDir.w = 0.0f;
					rayDir = invView * rayDir;

					Scene* pScene = SceneMgr::Get().GetActiveScene();

					SceneRayHit hit = {};
					if (pScene->Raycast(pCamData->GetPosition(), glm::normalize(glm::vec3(rayDir)), hit))
					{
						_selNodeName = hit.pHitNode->GetNode()->GetName();
						//spdlog::info("Hit: {}, pos: {},{},{}, norm: {},{},{}", hit.pHitNode->GetNode()->GetName(), hit.position.x, hit.position.y, hit.position.z, hit.normal.x, hit.normal.y, hit.normal.z);
					}

					break;
				}
			}
		}

		_renderer->SetCascadeSplitLambda(_settings.shadows.cascadeSplitLambda);

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

		if (!CreateRenderPassData(DefaultShaders::MSAAResolve, _msaaResolveData))
			return false;

		if (!CreateRenderPassData(DefaultShaders::Deferred, _deferredData))
			return false;

		if (!CreateRenderPassData(DefaultShaders::SceneCopy, _deferredCopyData))
			return false;

#ifdef SUPPORT_SSR
		if (!CreateRenderPassData(DefaultShaders::ScreenSpaceReflection, _ssrData))
			return false;
#endif

		_settings.shadows.enabled = EngineInfo::GetRenderer().ShadowsEnabled();

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

		rtInfo.msaa = EngineInfo::GetRenderer().GetMSAAMode();
		if (rtInfo.msaa != SE_MSAA_OFF)
		{
			if (!_msaaTarget.Create(rtInfo))
				return false;
			if (!_msaaResolveData.bindings.SetTexture(MaterialStrings::DiffuseMap, _msaaTarget.GetColorTexture()))
				return false;
			if (!_msaaResolveData.bindings.SetTexture(MaterialStrings::DepthMap, _msaaTarget.GetDepthTexture()))
				return false;
		}
		rtInfo.msaa = SE_MSAA_OFF;

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
		_ssrData.bindings.SetTexture(MaterialStrings::DiffuseMap, _deferredResolveTarget.GetColorTexture(0)); //Feed in previous frame results
		_ssrData.bindings.SetTexture(MaterialStrings::SpecularMap, _deferredTarget.GetColorTexture(1)); //used for blend factor blend
		_ssrData.bindings.SetTexture(MaterialStrings::NormalMap, _deferredTarget.GetColorTexture(2));
		_ssrData.bindings.SetTexture(MaterialStrings::PositionMap, _deferredTarget.GetColorTexture(3));
		_ssrData.bindings.SetTexture(MaterialStrings::DepthMap, _deferredTarget.GetDepthTexture());
#endif

		rtInfo.numTargets = 1;
		rtInfo.hasDepthBuffer = false;
		rtInfo.floatingPointColorBuffer = false;
		if (!_toneMapTarget.Create(rtInfo))
			return false;

		if (!_fxaaData.bindings.SetTexture(MaterialStrings::DiffuseMap, _toneMapTarget.GetColorTexture()))
		{
			//return false;
		}

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

	void SceneView::BuildSelectedNodeGUI(Scene* pScene, GUIRenderer* pRenderer)
	{
		bool removeNode = false;

		GameEditorGUI* gui = static_cast<GameEditorGUI*>(pRenderer);


		ImGui::BeginChild("SelNodeEditor");
		SceneNode* pNode = pScene->GetNode(_selNodeName);
		if (pNode)
		{
			ImGui::InputText("##Name", const_cast<char*>(pNode->GetName().c_str()), pNode->GetName().size(), ImGuiInputTextFlags_ReadOnly);
			if (ImGui::Button("Remove"))
			{
				removeNode = true;
			}

			bool visible = pNode->GetVisible();
			if (ImGui::Checkbox("Visible", &visible)) pNode->SetVisible(visible);

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
				else if (c->GetType() == COMPONENT_RENDER_OBJECT)
				{
					RenderComponentData* renderData = pNode->GetComponentData<RenderComponentData>(c);
					RenderObject* pRenderObject = const_cast<RenderObject*>(renderData->C()->As<RenderObject>());

					if (pRenderObject->GetRenderType() == RO_TERRAIN)
					{
						if (ImGui::TreeNode("Terrain"))
						{
							BuildTerrain(static_cast<Terrain*>(pRenderObject));
							ImGui::TreePop();
						}
					}

					HashSet<Material*> materials;
					for (auto riter = renderData->BeginNode(); riter != renderData->EndNode(); ++riter)
					{
						Material* pMaterial = (*riter).GetMaterial();
						if (pMaterial)
							materials.insert(pMaterial);
					}

					if(ImGui::TreeNode("Materials"))
					{
						for (auto& mtl : materials)
						{
							gui->RenderMaterial(mtl);
						}
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

	void SceneView::BuildTerrain(Terrain* pTerrain)
	{
		Vector<Terrain::Biome*> biomes;
		pTerrain->GetBiomes(biomes);

		for (auto& biome : biomes)
		{
			if (ImGui::TreeNode(biome->GetName().c_str()))
			{
				//void SetTexture(Texture2D * pTexture);
				//Texture2D* GetTexture() const { return _texture; }

				float ResolutionScale = biome->GetResolutionScale();
				if (ImGui::DragFloat("ResolutionScale", &ResolutionScale, 0.005f, 0.0f, 1.0f)) biome->SetResolutionScale(ResolutionScale);

				float HeightScale = biome->GetHeightScale();
				if (ImGui::DragFloat("HeightScale", &HeightScale, 0.5f)) biome->SetHeightScale(HeightScale);

				float HeightOffset = biome->GetHeightOffset();
				if (ImGui::DragFloat("HeightOffset", &HeightOffset, 0.5f)) biome->SetHeightOffset(HeightOffset);

				int SmoothKernelSize = biome->GetSmoothKernelSize();
				if (ImGui::DragInt("SmoothKernelSize", &SmoothKernelSize, 0.005f, 0, 3)) biome->SetSmoothKernelSize(SmoothKernelSize);

				bool Invert = biome->GetInvert();
				if (ImGui::Checkbox("Invert", &Invert)) biome->SetInvert(Invert);

				glm::int2 Center = biome->GetCenter();
				int halfRes = pTerrain->GetResolution() / 2;
				if (ImGui::DragInt2("Center", &Center.x, 0.5f, -halfRes, halfRes)) biome->SetCenter(Center);

				if (ImGui::Button("Update")) pTerrain->UpdateBiomes();

				ImGui::TreePop();
			}
		}
	}

	bool SceneView::CreateRenderPassData(const String& shader, RenderPassData& data, bool useOneZ)
	{
		BaseShader* pShader = !useOneZ ? ShaderMgr::Get().GetShader(shader)->GetBase() : ShaderMgr::Get().GetShader(shader)->GetBaseVariant(ShaderVariant::ONE_Z);
		assert(pShader);

		if (!pShader)
			return false;

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.pShader = pShader;
		bindingInfo.type = SBT_MATERIAL;

		if (!data.bindings.Create(bindingInfo))
			return false;

		FilterMode filter = SE_FM_NEAREST;

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
			//pipelineInfo.settings.blendState.srcAlphaBlendFactor = SE_BF_ONE;
			//pipelineInfo.settings.blendState.dstAlphaBlendFactor = SE_BF_ZERO;
			//pipelineInfo.settings.blendState.srcColorBlendFactor = SE_BF_SRC_ALPHA;
			//pipelineInfo.settings.blendState.dstColorBlendFactor = SE_BF_ONE_MINUS_SRC_ALPHA;
			//pipelineInfo.settings.depthStencil.depthCompareOp = se; //we only want to test pixels which have depths < 1, the quad will have depth == 1, so pixels which are the background wont be tested
		}
		else if (shader == DefaultShaders::SceneCopy)
		{
			//this pipeline writes to depth buffer in pixel shader
			pipelineInfo.settings.depthStencil.enableDepthWrite = true;
		}
		else if (shader == DefaultShaders::MSAAResolve)
		{
			//this pipeline writes to depth buffer in pixel shader
			pipelineInfo.settings.depthStencil.enableDepthWrite = true;
		}
		else if (shader == DefaultShaders::FXAA)
		{
			//fxaa needs linear filter for pixel averaging
			filter = SE_FM_LINEAR;
		}

		pipelineInfo.pShader = pShader;
		if (!data.pipeline.Create(pipelineInfo))
			return false;

		data.bindings.SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(filter, SE_WM_CLAMP_TO_EDGE));

		return true;
	}

	SceneView::Settings::Settings()
	{
		fxaa.enabled = true;
		fxaa.subpixel = 0.75f;
		fxaa.edgeThreshold = 0.166f;
		fxaa.edgeThresholdMin = 0.0833f;

		msaa.enabled = true;

		shadows.enabled = true;
		shadows.cascadeSplitLambda = 0.95f;

		memset(&gui, 0x0, sizeof(gui));
	}

	ShadowMapView::ShadowMapView(SceneRenderer* pRenderer) : View("ShadowMapView")
	{
		_renderer = pRenderer;
		SetCameraMode(CM_STATIC);
	}

	bool ShadowMapView::OnCreate(const CreateInfo&)
	{
		Shader* pShader = ShaderMgr::Get().GetShader(DefaultShaders::TextureCopy);

		GraphicsPipeline::CreateInfo pipelineInfo = {};
		pipelineInfo.pShader = pShader->GetBase();
		pipelineInfo.settings.rasterizer.cullMode = SE_CM_NONE;
		if (!_pipeline.Create(pipelineInfo))
			return false;

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.pShader = pipelineInfo.pShader;
		bindingInfo.type = SBT_MATERIAL;
		if (!_bindings.Create(bindingInfo))
			return false;

		if (!_bindings.SetTexture(MaterialStrings::DiffuseMap, _renderer->GetShadowMapTexture() ? _renderer->GetShadowMapTexture() : ResourceMgr::Get().GetTexture2D(DefaultResource::Texture::White)->GetGPUObject()))
			return false;
		if (!_bindings.SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE)))
			return false;

		return true;
	}

	bool ShadowMapView::Render(CommandBuffer* cmdBuffer)
	{
		auto env = SceneMgr::Get().GetActiveScene()->GetEnvironmentList().front();
		auto color = env->C()->As<Environment>()->GetActiveSkyModel()->GetSkyColor();

		_target.SetClearColor(color.r, color.g, color.b, 1);
		_target.Bind(cmdBuffer);
		//TODO BROKEN SINCE USING TEXTURE ARRAYS NOW...
		//BaseShader* pShader = _pipeline.GetShader();
		//pShader->Bind(cmdBuffer);
		//_pipeline.Bind(cmdBuffer);
		//_bindings.Bind(cmdBuffer);
		//cmdBuffer->Draw(6, 1, 0, 0);
		//_bindings.Unbind(cmdBuffer);
		//_pipeline.Unbind(cmdBuffer);
		//pShader->Unbind(cmdBuffer);
		_target.Unbind(cmdBuffer);

		return true;
	}


	Texture2DView::Texture2DView() : View("TextureView")
	{
		SetCameraMode(CM_STATIC);
	}

	bool Texture2DView::OnCreate(const CreateInfo&)
	{
		Shader* pShader = ShaderMgr::Get().GetShader(DefaultShaders::TextureCopy);

		GraphicsPipeline::CreateInfo pipelineInfo = {};
		pipelineInfo.pShader = pShader->GetBase();
		pipelineInfo.settings.rasterizer.cullMode = SE_CM_NONE;
		if (!_pipeline.Create(pipelineInfo))
			return false;

		ShaderBindings::CreateInfo bindingInfo = {};
		bindingInfo.pShader = pipelineInfo.pShader;
		bindingInfo.type = SBT_MATERIAL;
		if (!_bindings.Create(bindingInfo))
			return false;

		if (!_bindings.SetTexture(MaterialStrings::DiffuseMap, ResourceMgr::Get().GetTexture2D(DefaultResource::Texture::White)->GetGPUObject()))
			return false;
		if (!_bindings.SetSampler(MaterialStrings::Sampler, ResourceMgr::Get().GetSampler(SE_FM_LINEAR, SE_WM_CLAMP_TO_EDGE)))
			return false;

		return true;
	}

	bool Texture2DView::Render(CommandBuffer* cmdBuffer)
	{
		_target.SetClearColor(0, 1, 0, 1);
		_target.Bind(cmdBuffer);
		BaseShader* pShader = _pipeline.GetShader();
		pShader->Bind(cmdBuffer);
		_pipeline.Bind(cmdBuffer);
		_bindings.Bind(cmdBuffer);
		cmdBuffer->Draw(6, 1, 0, 0);
		_bindings.Unbind(cmdBuffer);
		_pipeline.Unbind(cmdBuffer);
		pShader->Unbind(cmdBuffer);
		_target.Unbind(cmdBuffer);

		return true;
	}

	void Texture2DView::RenderGUI(GUIRenderer* pRenderer)
	{
		class Texture2DViewUpdateCommand : public GUIRenderer::UpdateCommand
		{
		public:
			Texture2DViewUpdateCommand(Texture2D* pTex, ShaderBindings* pBindings)
			{
				_tex = pTex;
				_bindings = pBindings;
			}

			bool Execute() override
			{
				_bindings->SetTexture(MaterialStrings::DiffuseMap, _tex->GetGPUObject());
				return true;
			}

		private:
			Texture2D* _tex;
			ShaderBindings* _bindings;
		};

		ImGui::TableNextColumn();	
		ResourceMgr& resMgr = ResourceMgr::Get();

		auto iter = resMgr.IterTextures2D();
		while (!iter.End())
		{
			Texture2D* pTexture = *iter;
			if (pTexture->GetGPUObject()->GetAPIHandle() && ImGui::Button(GetFileName(pTexture->GetName().c_str()).c_str()))
			{
				Texture2DViewUpdateCommand* cmd = new Texture2DViewUpdateCommand(pTexture, &_bindings);
				pRenderer->PushUpdateCommand(cmd);
			}
			++iter;
		}
	}
}