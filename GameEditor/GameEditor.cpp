#include <spdlog/spdlog.h>
#include "glm/gtc/random.hpp"

#include "FileReader.h"
#include "Light.h"
#include "MeshRenderer.h"
#include "Camera.h"
#include "ResourceMgr.h"
#include "ShaderCompiler.h"
#include "SceneMgr.h"
#include "Asset.h"
#include "AssetImporter.h"
#include "StringUtil.h"
#include "GameEditorViews.h"
#include "GameEditorGUI.h"

#include "GameEditor.h"

namespace SunEngine
{
	GameEditor::GameEditor()
	{
		_queuedAsset = 0;
	}

	GameEditor::~GameEditor()
	{

	}

	bool GameEditor::CustomInit(ConfigSection* pEditorConfig, GraphicsWindow* pWindow, GUIRenderer** ppOutGUI)
	{
		if (!CompileShaders())
		{
			spdlog::error("Failed to compile Shaders");
			return false;
		}

		//Create SceneView
		View::CreateInfo viewInfo = {};
		String sceneViewConfig = pEditorConfig->GetString("SceneView");
		if (!viewInfo.ParseConfigString(sceneViewConfig))
		{
			viewInfo.width = pWindow->Width() / 2;;
			viewInfo.height = pWindow->Height() / 2;
			viewInfo.visibile = true;
		}

		View* pView = new SceneView(&_sceneRenderer);
		if (!pView->Create(viewInfo))
		{
			spdlog::error("Failed to create {} RenderTarget: {}", pView->GetName().c_str(), pView->GetRenderTarget()->GetErrStr().c_str());
			return false;
		}
		AddView(pView);

		//String inputFile = GetFilenameFromConfig("GameFile");
		bool loadDefault = true;
		if (loadDefault)
		{
			if (!CreateDefaultScene())
			{
				spdlog::error("Failed to create Default Scene");
				return false;
			}
		}
		
		if (!_sceneRenderer.Init())
		{
			spdlog::error("Failed to init SceneRenderer");
			return false;
		}

		*ppOutGUI = new GameEditorGUI();
		spdlog::info("GameEditor initialization succesfull");
		return true;
	}

	Asset* GameEditor::ImportAsset(const String& filename, const AssetImporter::Options& options)
	{
		AssetImporter importer;
		if (importer.Import(filename, options))
		{
			_queuedAsset = importer.GetAsset();
			return _queuedAsset;
		}
		else
		{
			return 0;
		}
	}

	void GameEditor::CustomUpdate()
	{
		Scene* pScene = SceneMgr::Get().GetActiveScene();

		if (_queuedAsset)
		{
			_queuedAsset->CreateSceneNode(pScene);
			_queuedAsset = 0;
		}

		pScene->Update(1 / 60.0f, 0.0f);
	}

	bool GameEditor::CompileShaders()
	{
		String path = GetPathFromConfig("ShaderConfig");
		if (!path.length())
			return false;

		String shaderDir = GetPathFromConfig("Shaders");

		ConfigFile config;
		if (!config.Load(path.data()))
			return false;

		for (auto iter = config.Begin(); iter != config.End(); ++iter)
		{
			CompiledShaderInfo& info = _compiledShaders[(*iter).first];
			if (!CompileShader(info, (*iter).first))
				return false;
		}

		auto& resMgr = ResourceMgr::Get();
		resMgr.CreateDefaults();

		for (auto iter = _compiledShaders.begin(); iter != _compiledShaders.end(); ++iter)
		{
			Shader* pShader = resMgr.AddShader((*iter).first);
			pShader->SetCreateInfo((*iter).second.CreateInfo);
			if (!pShader->RegisterToGPU())
			{
				spdlog::error("Failed to create shader: {}", pShader->GetGPUObject()->GetErrStr().data());
				return false;
			}

			if ((*iter).second.ConfigPath.length())
			{
				if (!pShader->LoadConfig((*iter).second.ConfigPath))
				{
					spdlog::error("Failed to load shader config: {}", (*iter).second.ConfigPath.c_str());
					return false;
				}
			}
		}

		return true;
	}

	bool GameEditor::CreateDefaultScene()
	{
		auto& resMgr = ResourceMgr::Get();

		Shader* pMetalShader = resMgr.GetShader(DefaultResource::Shader::StandardMetallic);
		if (pMetalShader == 0)
		{
			spdlog::error("Failed to find {} Shader", DefaultResource::Shader::StandardMetallic.c_str());
			return false;
		}

		Material* pMetalMaterial = resMgr.AddMaterial(DefaultResource::Material::StandardMetallic);
		pMetalMaterial->SetShader(pMetalShader);
		if (!pMetalMaterial->RegisterToGPU())
		{
			spdlog::error("Failed to register {}: {}", pMetalMaterial->GetName().data(), pMetalMaterial->GetGPUObject()->GetErrStr().data());
			return false;
		}
		pMetalShader->SetDefaults(pMetalMaterial);

		Shader* pSpecularShader = resMgr.GetShader(DefaultResource::Shader::StandardSpecular);
		if (pSpecularShader == 0)
		{
			spdlog::error("Failed to find {} Shader", DefaultResource::Shader::StandardSpecular.c_str());
			return false;
		}

		Material* pSpecularMaterial = resMgr.AddMaterial(DefaultResource::Material::StandardSpecular);
		pSpecularMaterial->SetShader(pSpecularShader);
		if (!pSpecularMaterial->RegisterToGPU())
		{
			spdlog::error("Failed to register {}: {}", pSpecularMaterial->GetName().data(), pSpecularMaterial->GetGPUObject()->GetErrStr().data());
			return false;
		}
		pSpecularShader->SetDefaults(pSpecularMaterial);

		String testPath;
		testPath = "F:\\Models\\TerrainTextures\\large-metal-debris\\";
		String diffuseName = "large_metal_debris_Base_Color.jpg";
		String metalName = "large_metal_debris_Metallic.jpg";
		String roughnessName = "large_metal_debris_Glossiness.jpg";
		String normName = "large_metal_debris_Normal.jpg";
		String aoName = "large_metal_debris_Ambient_Occlusion.jpg";

		//testPath = "C:/Users/Bart/Documents/Code/LearnOpenGL-master/resources/textures/pbr/rusted_iron/";
		//diffuseName = "albedo.png";
		//normName = "normal.png";
		//metalName = "metallic.png";
		//roughnessName = "roughness.png";
		//aoName = "ao.png";

		//testPath = "C:/Users/Bart/Documents/Code/LearnOpenGL-master/resources/textures/pbr/grass/";
		//diffuseName = "albedo.png";
		//normName = "normal.png";
		//metalName = "metallic.png";
		//roughnessName = "roughness.png";
		//aoName = "ao.png";

		//testPath = "C:/Users/Bart/Documents/Code/LearnOpenGL-master/resources/textures/pbr/gold/";
		//diffuseName = "albedo.png";
		//normName = "normal.png";
		//metalName = "metallic.png";
		//roughnessName = "roughness.png";
		//aoName = "ao.png";

		//testPath = "C:/Users/Bart/Documents/Code/LearnOpenGL-master/resources/textures/pbr/wall/";
		//diffuseName = "albedo.png";
		//normName = "normal.png";
		//metalName = "metallic.png";
		//roughnessName = "roughness.png";
		//aoName = "ao.png";

		//testPath = "C:/Users/Bart/Documents/Code/LearnOpenGL-master/resources/textures/pbr/plastic/";
		//diffuseName = "albedo.png";
		//normName = "normal.png";
		//metalName = "metallic.png";
		//roughnessName = "roughness.png";
		//aoName = "ao.png";

		//Texture2D* pTexD = resMgr.AddTexture2D(diffuseName);
		//pTexD->LoadFromFile(testPath + diffuseName);
		//pTexD->GenerateMips();
		//pTexD->RegisterToGPU();

		//Texture2D* pTexM = resMgr.AddTexture2D(metalName);
		//pTexM->LoadFromFile(testPath + metalName);
		//pTexM->RegisterToGPU();

		//Texture2D* pTexR = resMgr.AddTexture2D(roughnessName);
		//pTexR->LoadFromFile(testPath + roughnessName);
		//if(StrContains((roughnessName), "gloss"))
		//	pTexR->Invert();
		//pTexR->GenerateMips();
		//pTexR->RegisterToGPU();

		//Texture2D* pTexAO = resMgr.AddTexture2D(aoName);
		//pTexAO->LoadFromFile(testPath + aoName);
		//pTexAO->GenerateMips();
		//pTexAO->RegisterToGPU();

		//Texture2D* pTexN = resMgr.AddTexture2D(normName);
		//pTexN->LoadFromFile(testPath + normName);
		//pTexN->GenerateMips();
		//pTexN->RegisterToGPU();

		//pBlinnPhongMaterial->SetTexture2D("DiffuseMap", pTexD);
		//pBlinnPhongMaterial->SetTexture2D("NormalMap", pTexN);
		////pBlinnPhongMaterial->SetTexture("SpecularMap", pTexD);

		//pStandardMaterial->SetTexture2D("DiffuseMap", pTexD);
		//pStandardMaterial->SetTexture2D("MetallicMap", pTexM);
		//pStandardMaterial->SetTexture2D("RoughnessMap", pTexR);
		//pStandardMaterial->SetTexture2D("AOMap", pTexAO);
		//pStandardMaterial->SetTexture2D("NormalMap", pTexN);

		auto& sceneMgr = SceneMgr::Get();
		Scene* pScene = sceneMgr.AddScene("NewScene");

		SceneNode* pCamNode = pScene->AddNode("Camera");
		Camera* pCamera = pCamNode->AddComponent(new Camera())->As<Camera>();
		pCamera->SetFrustum(45.0f, 1.0f, 0.1f, 500.0f);
		pCamera->SetRenderToWindow(true);

		SceneNode* pLightNode = pScene->AddNode("Sun");
		Light* pSun = pLightNode->AddComponent(new Light())->As<Light>();
		pSun->SetLightType(LT_DIRECTIONAL);
		pSun->SetColor(glm::vec4(1));
		//pLightNode->Orientation.Angles.y = 180.0f;

		Asset* pAssetStandard = resMgr.AddAsset("AssetStandard");
		{
			AssetNode* pRoot = pAssetStandard->AddNode("Root");
			MeshRenderer* pRenderer = pRoot->AddComponent(new MeshRenderer())->As<MeshRenderer>();
			pRenderer->SetMesh(resMgr.GetMesh(DefaultResource::Mesh::Sphere));
			pRenderer->SetMaterial(resMgr.Clone(pMetalMaterial));
			pRenderer->GetMaterial()->RegisterToGPU();
			
			SceneNode* pSceneNode = pAssetStandard->CreateSceneNode(pScene);
			pSceneNode->Position = glm::vec3(-3.0f, 0.0f, -10.0f);
		}

		Asset* pAssetBlinnPhong = resMgr.AddAsset("AssetBlinnPhong");
		{
			AssetNode* pRoot = pAssetBlinnPhong->AddNode("Root");
			MeshRenderer* pRenderer = pRoot->AddComponent(new MeshRenderer())->As<MeshRenderer>();
			pRenderer->SetMesh(resMgr.GetMesh(DefaultResource::Mesh::Sphere));
			pRenderer->SetMaterial(resMgr.Clone(pSpecularMaterial));
			pRenderer->GetMaterial()->RegisterToGPU();

			SceneNode* pSceneNode = pAssetBlinnPhong->CreateSceneNode(pScene);
			pSceneNode->Position = glm::vec3(+3.0f, 0.0f, -10.0f);
			//pSceneNode->Scale = glm::vec3(30, 0.01f, 30.0f);
		}

		Asset* pAssetPlane = resMgr.AddAsset("AssetPlane");
		{
			AssetNode* pRoot = pAssetPlane->AddNode("Root");
			MeshRenderer* pRenderer = pRoot->AddComponent(new MeshRenderer())->As<MeshRenderer>();
			pRenderer->SetMesh(resMgr.GetMesh(DefaultResource::Mesh::Plane));
			pRenderer->SetMaterial(resMgr.Clone(pSpecularMaterial));
			pRenderer->GetMaterial()->RegisterToGPU();

			SceneNode* pSceneNode = pAssetPlane->CreateSceneNode(pScene);
			pSceneNode->Position = glm::vec3(0.0f, -5.0f, -15.0f);
			pSceneNode->Scale = glm::vec3(30, 30.0f, 30.0f);
		}

		//int cubeSlices = 1;
		//float cubeOffset = 3.0f;
		//float cubeZStart = -(cubeSlices * cubeOffset * 2.0f);
		//float cubeXStart = -(cubeSlices * cubeOffset * 0.5f);

		//for (int i = 0; i < cubeSlices; i++) 
		//{
		//	for (int j = 0; j < cubeSlices; j++) 
		//	{
		//		for (int k = 0; k < cubeSlices; k++)
		//		{
		//			SceneNode* pCubeSceneNode = pCubeAsset->CreateSceneNode(pScene);
		//			pCubeSceneNode->Position = glm::vec3(i * cubeOffset, j * cubeOffset, k * cubeOffset);
		//			pCubeSceneNode->Position.z += cubeZStart;
		//			pCubeSceneNode->Position.x += cubeXStart;
		//			pCubeSceneNode->Orientation.Angles = glm::linearRand(glm::vec3(0.0f), glm::vec3(360.0f));
		//		}
		//	}
		//}

		//pScene->Initialize();
		sceneMgr.SetActiveScene(pScene->GetName());

		return true;
	}
}
