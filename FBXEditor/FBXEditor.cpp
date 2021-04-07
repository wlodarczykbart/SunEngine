#include "FBXImporter.h"
#include "spdlog/spdlog.h"
#include "FBXEditorGUI.h"
#include "StringUtil.h"
#include "ResourceMgr.h"
#include "Asset.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "CommandBuffer.h"
#include "AssetImporter.h"
#include "ShaderMgr.h"
#include "FilePathMgr.h"

#include "FBXEditor.h"

namespace SunEngine
{
	void CopyProps(FbxProperty prop, FbxProperty parent, uint level, FbxSurfacePhong* mtl)
	{
		while (prop.IsValid())
		{
			auto name = prop.GetName();

			for (uint i = 0; i < prop.GetSrcObjectCount(); i++)
			{
				auto srcObject = prop.GetSrcObject(i);
				mtl->FindProperty(name).ConnectSrcObject(srcObject);
			}

			//spdlog::info("level: {}, name: {}", level, name);

			CopyProps(prop.GetChild(), prop, level + 1, mtl);
			prop = parent.GetNextDescendent(prop);
		}
	}

	class FBXView : public View
	{
	public:
		FBXView() : View("FBXView")
		{
			_asset = 0;
		}

		bool Init()
		{
			Shader* pShader = ShaderMgr::Get().GetShader(DefaultShaders::Specular);
			GraphicsPipeline::CreateInfo pipelineInfo = {};
			pipelineInfo.pShader = pShader->GetDefault();

			if (!_opaquePipeline.Create(pipelineInfo))
				return false;

			UniformBuffer::CreateInfo uboInfo = {};

			uboInfo.isShared = true;
			uboInfo.size = sizeof(ObjectBufferData);
			if (!_objectBuffer.first.Create(uboInfo))
				return false;

			uboInfo.isShared = false;
			uboInfo.size = sizeof(CameraBufferData);
			if (!_cameraBuffer.first.Create(uboInfo))
				return false;

			uboInfo.isShared = false;
			uboInfo.size = sizeof(SunlightBufferData);
			if (!_lightBuffer.first.Create(uboInfo))
				return false;

			ShaderBindings::CreateInfo bindInfo = {};
			bindInfo.pShader = pShader->GetDefault();

			bindInfo.type = SBT_OBJECT;
			if (!_objectBuffer.second.Create(bindInfo))
				return false;
			_objectBuffer.second.SetUniformBuffer(ShaderStrings::ObjectBufferName, &_objectBuffer.first);

			bindInfo.type = SBT_CAMERA;
			if (!_cameraBuffer.second.Create(bindInfo))
				return false;
			_cameraBuffer.second.SetUniformBuffer(ShaderStrings::CameraBufferName, &_cameraBuffer.first);

			bindInfo.type = SBT_LIGHT;
			if (!_lightBuffer.second.Create(bindInfo))
				return false;
			_lightBuffer.second.SetUniformBuffer(ShaderStrings::SunlightBufferName, &_lightBuffer.first);

			return true;
		}

		void SetAsset(Asset* pAsset)
		{
			_renderData.clear();
			_scene.Clear();
			if (pAsset)
			{
				SceneNode* pNode = pAsset->CreateSceneNode(&_scene);
				pNode->Traverse([](SceneNode* pNode, void* pData) -> bool
				{
					pNode->Initialize();
					pNode->Update(0.0f, 0.0f);

					Vector<Component*> meshRenderers;
					pNode->GetComponentsOfType(COMPONENT_MESH_RENDERER, meshRenderers);

					Vector<const RenderNode*>* pRenderNodeList = static_cast<Vector<const RenderNode*>*>(pData);
					for (uint i = 0; i < meshRenderers.size(); i++)
					{
						MeshRendererComponentData* pRenderData = pNode->GetComponentData<MeshRendererComponentData>(meshRenderers[i]);
						for (auto iter = pRenderData->BeginNode(); iter != pRenderData->EndNode(); ++iter)
						{
							pRenderNodeList->push_back(&(*iter));
						}
					}
					return true;
				}, &_renderData);

				Vector<ObjectBufferData> vBuffer;
				for (uint i = 0; i < _renderData.size(); i++)
				{
					ObjectBufferData data = {};
					data.WorldMatrix.Set(&_renderData[i]->GetWorld());
					glm::mat4 itp = glm::transpose(glm::inverse(_renderData[i]->GetWorld()));
					data.InverseTransposeMatrix.Set(&itp);
					vBuffer.push_back(data);
				}
				_objectBuffer.first.UpdateShared(vBuffer.data(), vBuffer.size());
			}
			_asset = pAsset;
		}

		bool Render(CommandBuffer* cmdBuffer) override
		{
			if(_asset == 0)
				return View::Render(cmdBuffer);

			_target.Bind(cmdBuffer);

			_opaquePipeline.GetShader()->Bind(cmdBuffer);
			_opaquePipeline.Bind(cmdBuffer);

			glm::mat4 proj = GetProjMatrix();
			glm::mat4 invProj = glm::inverse(proj);
			glm::mat4 view = GetViewMatrix();
			glm::mat4 invView = glm::inverse(view);

			CameraBufferData camBuff = {};
			camBuff.ViewMatrix.Set(&view);
			camBuff.ProjectionMatrix.Set(&proj);
			camBuff.InvViewMatrix.Set(&invView);
			camBuff.InvProjectionMatrix.Set(&invProj);
			_cameraBuffer.first.Update(&camBuff);
			_cameraBuffer.second.Bind(cmdBuffer);

			SunlightBufferData sunBuff = {};
			sunBuff.Color.Set(1, 1, 1, 1);
			sunBuff.Direction.Set(0, 1, 0, 0);
			_lightBuffer.first.Update(&sunBuff);
			_lightBuffer.second.Bind(cmdBuffer);

			IShaderBindingsBindState bindState = {};
			bindState.DynamicIndices[0] = { ShaderStrings::ObjectBufferName, 0 };

			for (uint i = 0; i < _renderData.size(); i++)
			{
				bindState.DynamicIndices[0].second = i;
				_objectBuffer.second.Bind(cmdBuffer, &bindState);

				const RenderNode* pNode = _renderData[i];
				pNode->GetMesh()->GetGPUObject()->Bind(cmdBuffer);
				pNode->GetMaterial()->GetGPUObject()->Bind(cmdBuffer);
				cmdBuffer->DrawIndexed(
					pNode->GetIndexCount(),
					pNode->GetInstanceCount(),
					pNode->GetFirstIndex(),
					pNode->GetVertexOffset(),
					0);
			}

			_target.Unbind(cmdBuffer);
			return true;
		}


	private:
		struct RenderData
		{
			AssetNode* pNode;
			MeshRenderer* pRenderer;
		};

		Asset* _asset;
		Scene _scene;
		Vector<const RenderNode*> _renderData;

		GraphicsPipeline _opaquePipeline;
		GraphicsPipeline _alphaPipeline;
		Pair<UniformBuffer, ShaderBindings> _objectBuffer;
		Pair<UniformBuffer, ShaderBindings> _cameraBuffer;
		Pair<UniformBuffer, ShaderBindings> _lightBuffer;
	};

	FBXEditor::FBXEditor()
	{
		_actionFlags = 0;
		_view = 0;
		_fbxManager = 0;
		_fbxImporter = 0;
		_fbxExporter = 0;
		_fbxScene = 0;
		_currentAsset = 0;
	}

	bool FBXEditor::CustomParseConfig(ConfigFile* pConfig)
	{
		EngineInfo::Init(pConfig);
	}

	bool FBXEditor::CustomLoad(GraphicsWindow* pWindow, GUIRenderer** ppOutGUI)
	{

		ResourceMgr& resMgr = ResourceMgr::Get();
		ShaderMgr& shaderMgr = ShaderMgr::Get();

		if (!resMgr.CreateDefaults())
		{
			spdlog::error("Failed to create default resource");
			return false;
		}

		if (!shaderMgr.LoadShaders())
		{
			spdlog::error("Failed to load shaders");
			return false;
		}

		Shader* pShader = shaderMgr.GetShader(DefaultShaders::Specular);
		if (!pShader)
		{
			spdlog::error("Failed to find {} shader", DefaultShaders::Specular);
			return false;
		}

		Material* pMaterial = resMgr.AddMaterial(DefaultResource::Material::StandardSpecular);
		pMaterial->SetShader(pShader);
		if (!pMaterial->RegisterToGPU())
			return false;
		pShader->SetDefaults(pMaterial);

		_view = new FBXView();
		View::CreateInfo viewInfo = {};
		viewInfo.width = pWindow->Width()/2;
		viewInfo.height = pWindow->Height()/2;
		viewInfo.visibile = true;
		if (!_view->Create(viewInfo))
			return false;
		if (!_view->Init())
			return false;
		AddView(_view);

		_fbxManager = FbxManager::Create();
		if (!_fbxManager)
		{
			spdlog::error("Failed to create FbxManager");
			return false;
		}

		*ppOutGUI = new FBXEditorGUI();
		return true;
	}

	void FBXEditor::CustomUpdate()
	{
		if (_actionFlags & AF_IMPORT)
		{
			String path;
			if(Editor::SelectFile(path, "FBX File", "*.fbx"))
			{
				Import(path);
			}
		}

		if (_actionFlags & AF_EXPORT)
		{
			String path;
			if (_fbxScene && Editor::SelectFile(path, "FBX File", "*.fbx"))
			{
				if (path != _fbxImporter->GetFileName().Buffer())
					Export(path);
				else
					spdlog::warn("Export path cannot be same as Import path...");
			}
		}

		if (_actionFlags & AF_PREVIEW)
		{
			if (_currentExportFile.length())
			{
				if (_currentAsset)
				{
					ResourceMgr::Get().Remove(_currentAsset);
				}

				AssetImporter importer;
				if (importer.Import(_currentExportFile))
				{
					_currentAsset = importer.GetAsset();
					_view->SetAsset(_currentAsset);
				}
			}
		}

		if(_actionFlags != 0)
			_actionFlags = 0;
	}

	bool FBXEditor::Import(const String& path)
	{
		if (_fbxScene)
		{
			_fbxScene->Destroy();
			_fbxScene = 0;
		}

		if (_fbxImporter)
		{
			_fbxImporter->Destroy();
			_fbxImporter = 0;
		}
		
		_fbxImporter = FbxImporter::Create(_fbxManager, "");
		if (!_fbxImporter)
		{
			spdlog::error("Failed to create FbxImporter");
			return false;

		}

		if (!_fbxImporter->Initialize(path.c_str()))
		{
			spdlog::error("Failed to initialize %s import", path.c_str());
			return false;
		}

		_fbxScene = FbxScene::Create(_fbxManager, "");
		if (!_fbxScene)
		{
			spdlog::error("Failed to create FbxScene");
			return false;

		}

		if (!_fbxImporter->Import(_fbxScene))
		{
			spdlog::error("Failed to import FbxScene");
			return false;
		}

		FbxArray<FbxSurfaceMaterial*> materials;
		_fbxScene->FillMaterialArray(materials);

		Map<FbxNode*, Map<uint, FbxSurfaceMaterial*>> materialRemap;
		Vector<Pair<FbxSurfaceMaterial*, FbxSurfaceMaterial*>> materialList;
		for (int i = 0; i < materials.GetCount(); i++)
		{
			FbxSurfaceMaterial* oldMtl = materials.GetAt(i);
			if (!oldMtl->Is<FbxSurfacePhong>())
			{
				FbxSurfacePhong* phong = FbxSurfacePhong::Create(_fbxManager, oldMtl->GetName());
				phong = (FbxSurfacePhong*)&phong->Copy(*oldMtl);

				if (oldMtl->Is<FbxSurfaceLambert>())
				{
					FbxSurfaceLambert* lambert = static_cast<FbxSurfaceLambert*>(oldMtl);
					phong->Emissive.Set(lambert->				 Emissive);
					phong->EmissiveFactor.Set(lambert->			 EmissiveFactor);
					phong->Ambient.Set(lambert->					 Ambient);
					phong->AmbientFactor.Set(lambert->			 AmbientFactor);
					phong->Diffuse.Set(lambert->					 Diffuse);
					phong->DiffuseFactor.Set(lambert->			 DiffuseFactor);
					phong->NormalMap.Set(lambert->				 NormalMap);
					phong->Bump.Set(lambert->					 Bump);
					phong->BumpFactor.Set(lambert->				 BumpFactor);
					phong->TransparentColor.Set(lambert->		 TransparentColor);
					phong->TransparencyFactor.Set(lambert->		 TransparencyFactor);
					phong->DisplacementColor.Set(lambert->		 DisplacementColor);
					phong->DisplacementFactor.Set(lambert->		 DisplacementFactor);
					phong->VectorDisplacementColor.Set(lambert->	 VectorDisplacementColor);
					phong->VectorDisplacementFactor.Set(lambert-> VectorDisplacementFactor);

					auto root = lambert->RootProperty;
					auto child = root.GetChild();
					CopyProps(root, child, 0, phong);


					//phong->FindProperty(ModelImporter::FBXImporter::FBX_DIFFUSE_MAP).
					//	ConnectSrcObject(lambert->FindProperty(ModelImporter::FBXImporter::FBX_DIFFUSE_MAP).GetSrcObject<FbxTexture>());
				}

				for (int j = 0; j < _fbxScene->GetNodeCount(); j++)
				{
					FbxNode* pNode = _fbxScene->GetNode(j);

					for(int k = 0; k  < pNode->GetMaterialCount(); k ++)
					{ 
						if (pNode->GetMaterial(k) == oldMtl)
						{
							materialRemap[pNode][k] = phong;
						}
					}
				}
				materialList.push_back({ oldMtl, phong });
			}
		}

#if 1
		//fill in material remap with materials that will stick around
		for (auto iter = materialRemap.begin(); iter != materialRemap.end(); ++iter)
		{
			FbxNode* pNode = (*iter).first;
			for (int i = 0; i < pNode->GetMaterialCount(); i++)
			{
				auto found = (*iter).second.find(i);
				if (found == (*iter).second.end())
				{
					(*iter).second[i] = pNode->GetMaterial(i);
				}
			}
		}

		//remove old materials and add new ones to scene
		for (uint i = 0; i < materialList.size(); i++)
		{
			bool removed = _fbxScene->RemoveMaterial(materialList[i].first);
			assert(removed);
			bool added = _fbxScene->AddMaterial(materialList[i].second);
			assert(added);

			materialList[i].first->Destroy();
		}

		for (auto iter = materialRemap.begin(); iter != materialRemap.end(); ++iter)
		{
			FbxNode* pNode = (*iter).first;
			Vector<FbxSurfaceMaterial*> nodeMaterials;
			nodeMaterials.resize((*iter).second.size());
			
			for (auto mtlIter = (*iter).second.begin(); mtlIter != (*iter).second.end(); ++mtlIter)
			{
				nodeMaterials[(*mtlIter).first] = (*mtlIter).second;
			}
			
			pNode->RemoveAllMaterials();
			for (uint i = 0; i < nodeMaterials.size(); i++)
			{
				pNode->AddMaterial(nodeMaterials[i]);
			}
		}
#endif

		_currentExportFile = path;
		return true;
	}

	bool FBXEditor::Export(const String& path)
	{
		if (_fbxExporter)
		{
			_fbxExporter->Destroy();
			_fbxExporter = 0;
		}

		_fbxExporter = FbxExporter::Create(_fbxManager, "");
		if (!_fbxExporter)
		{
			spdlog::error("Failed to create FbxExporter");
			return false;

		}

		if (!_fbxExporter->Initialize(path.c_str()))
		{
			spdlog::error("Failed to initialize %s export", path.c_str());
			return false;
		}

		if (!_fbxExporter->Export(_fbxScene))
		{
			spdlog::error("Failed to export FbxScene");
		}

		_fbxExporter->Destroy();
		_fbxExporter = 0;

		_currentExportFile = path;
		return true;
	}

	glm::vec3 FromFbxDouble(const FbxDouble3& value)
	{
		return glm::vec3((float)value[0], (float)value[1], (float)value[2]);
	}

	glm::vec2 FromFbxDouble(const FbxDouble2& value)
	{
		return glm::vec2((float)value[0], (float)value[1]);
	}

	float FromFbxDouble(const FbxDouble& value)
	{
		return (float)value;
	}

	FbxDouble3 ToFbxDouble(const glm::vec3& value)
	{
		return FbxDouble3(value.x, value.y, value.z);
	}

	FbxDouble2 ToFbxDouble(const glm::vec2& value)
	{
		return FbxDouble2(value.x, value.y);
	}
}