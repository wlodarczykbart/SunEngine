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
#include "ShaderMgr.h"
#include "GameEditorGUI.h"
#include "Environment.h"
#include "Timer.h"
#include "FilePathMgr.h"

#include "GameEditor.h"

namespace SunEngine
{
	Vector<SceneNode*> TEST_NODES;

	GameEditor::GameEditor()
	{
	}

	GameEditor::~GameEditor()
	{

	}

	bool GameEditor::CustomParseConfig(ConfigFile* pConfig)
	{
		EngineInfo::Init(pConfig);
		return true;
	}

	bool GameEditor::CustomLoad(GraphicsWindow* pWindow, GUIRenderer** ppOutGUI)
	{
		ResourceMgr& resMgr = ResourceMgr::Get();
		ShaderMgr& shaderMgr = ShaderMgr::Get();

		if (!resMgr.CreateDefaults())
		{
			spdlog::error("Failed to create default resource");
			return false;
		}

		String shaderErr;
		if (!shaderMgr.LoadShaders(shaderErr))
		{
			spdlog::error("Failed to load shaders: \n{}", shaderErr);
			return false;
		}

		//TODO?
		//ConfigSection* pEditorConfig = pConfig->GetSection("Editor");

		//Create SceneView
		View::CreateInfo viewInfo = {};
		//String sceneViewConfig = pEditorConfig->GetString("SceneView");
		//if (!viewInfo.ParseConfigString(sceneViewConfig))
		{
			viewInfo.width = pWindow->Width() / 2;
			viewInfo.height = pWindow->Height() / 2;
			viewInfo.visibile = true;
			viewInfo.floatingPointColorBuffer = false;
		}

		_view = new SceneView(&_sceneRenderer);
		//_view->SetRenderToGraphicsWindow(true);
		if (!_view->Create(viewInfo))
		{
			spdlog::error("Failed to create {} RenderTarget: {}", _view->GetName().c_str(), _view->GetRenderTarget()->GetErrStr().c_str());
			return false;
		}
		AddView(_view);

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

		GameEditorGUI* gui = new GameEditorGUI();
		gui->RegisterSceneView(_view);
		*ppOutGUI = gui;
		spdlog::info("GameEditor initialization succesfull");
		return true;
	}

	Asset* GameEditor::ImportAsset(const String& filename, const AssetImporter::Options& options)
	{
		AssetImporter importer;
		if (importer.Import(filename, options))
		{
			return importer.GetAsset();
		}
		else
		{
			return 0;
		}
	}

	void GameEditor::CustomUpdate()
	{
		Scene* pScene = SceneMgr::Get().GetActiveScene();
		for (SceneNode* node : TEST_NODES)
		{
			node->Orientation.Angles.y += 5.0f;

		}

		static Timer timer(true);

		float dt = (float)timer.Tick();
		pScene->Update(dt, (float)timer.ElapsedTime());
	}

	bool GameEditor::CreateDefaultScene()
	{
		auto& resMgr = ResourceMgr::Get();
		auto& shaderMgr = ShaderMgr::Get();

		Shader* pMetalShader = shaderMgr.GetShader(DefaultShaders::Metallic);
		if (pMetalShader == 0)
		{
			spdlog::error("Failed to find {} Shader", DefaultShaders::Metallic.c_str());
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

		Shader* pSpecularShader = shaderMgr.GetShader(DefaultShaders::Specular);
		if (pSpecularShader == 0)
		{
			spdlog::error("Failed to find {} Shader", DefaultShaders::Specular.c_str());
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

		SceneNode* pEnvironmentNode = pScene->AddNode("Environment");
		auto* pEnv = pEnvironmentNode->AddComponent(new Environment())->As<Environment>();
		pEnvironmentNode->Initialize();

		const ConfigSection* pEnvSection = GetConfig().GetSection("Environment");
		if (pEnvSection)
		{
			String skyPath = pEnvSection->GetString("Skybox");
			String skyOrder = pEnvSection->GetString("SkyboxOrder");
			Vector<String> skySides;
			StrSplit(skyOrder, skySides, ',');
			if (!skyPath.empty() && skySides.size() == 6)
			{
				TextureCube* pSkyCube = resMgr.AddTextureCube("DefaultSkybox");
				String path = GetDirectory(GetConfig().GetFilename()) + "/" + skyPath + "/";
				SkyModelSkybox* pSkybox = static_cast<SkyModelSkybox*>(pEnv->GetSkyModel(DefaultShaders::Skybox));
				if (!(pSkyCube->LoadFromFile(path, skySides) && pSkyCube->RegisterToGPU() && pSkybox->SetSkybox(pSkyCube)))
				{
					spdlog::error("Failed to load skybox located at {}", path.c_str());
					return false;
				}
			}

			String cloudPath = pEnvSection->GetString("Clouds");
			if (!cloudPath.empty())
			{
				Texture2D* pCloudTex = resMgr.AddTexture2D("DefaultClouds");
				String path = GetDirectory(GetConfig().GetFilename()) + "/" + cloudPath;
				pCloudTex->SetFilename(path);
				if (!(pCloudTex->LoadFromFile() && pCloudTex->RegisterToGPU() && pEnv->SetCloudTexture(pCloudTex)))
				{
					spdlog::error("Failed to load clouds located at {}", path.c_str());
					return false;
				}
			}
		}

		Asset* pAssetStandard = resMgr.AddAsset("AssetStandard");
		{
			AssetNode* pRoot = pAssetStandard->AddNode("Cube");
			MeshRenderer* pRenderer = pRoot->AddComponent(new MeshRenderer())->As<MeshRenderer>();
			pRenderer->SetMesh(resMgr.GetMesh(DefaultResource::Mesh::Cube));
			pRenderer->SetMaterial(resMgr.Clone(pMetalMaterial));
			pRenderer->GetMaterial()->RegisterToGPU();
			
			SceneNode* pSceneNode = pAssetStandard->CreateSceneNode(pScene);
			pSceneNode->Position = glm::vec3(2.0f, 0.5f, 1.0f);
			pSceneNode->Orientation.Angles = glm::vec3(65, 55, -125);
		}

		Asset* pAssetBlinnPhong = resMgr.AddAsset("AssetBlinnPhong");
		{
			AssetNode* pRoot = pAssetBlinnPhong->AddNode("Sphere");
			MeshRenderer* pRenderer = pRoot->AddComponent(new MeshRenderer())->As<MeshRenderer>();
			pRenderer->SetMesh(resMgr.GetMesh(DefaultResource::Mesh::Sphere));
			pRenderer->SetMaterial(resMgr.Clone(pSpecularMaterial));
			pRenderer->GetMaterial()->RegisterToGPU();

			SceneNode* pSceneNode = pAssetBlinnPhong->CreateSceneNode(pScene);
			pSceneNode->Position = glm::vec3(+0.0f, 1.5f, 0.0f);
			//pSceneNode->Scale = glm::vec3(30, 0.01f, 30.0f);
		}

		Asset* pAssetPlane = resMgr.AddAsset("AssetPlane");
		{
			AssetNode* pRoot = pAssetPlane->AddNode("Plane");
			MeshRenderer* pRenderer = pRoot->AddComponent(new MeshRenderer())->As<MeshRenderer>();
			pRenderer->SetMesh(resMgr.GetMesh(DefaultResource::Mesh::Plane));
			Material* pPlaneMaterial = resMgr.Clone(pSpecularMaterial);
			pRenderer->SetMaterial(pPlaneMaterial);
			pRenderer->GetMaterial()->RegisterToGPU();
			pPlaneMaterial->SetTexture2D(MaterialStrings::DiffuseMap, resMgr.GetTexture2D(DefaultResource::Texture::Default));

			SceneNode* pSceneNode = pAssetPlane->CreateSceneNode(pScene);
			pSceneNode->Position = glm::vec3(0.0f, 0.0f, 0.0f);
			pSceneNode->Scale = glm::vec3(30, 30.0f, 30.0f);
		}

		int Slices = 0;
		float Offset = 3.0f;
		float ZStart = -(Slices * Slices * 2.0f);
		float XStart = -(Slices * Slices * 0.5f);

		float halfOffset = (Slices / 2) * Offset;

		for (int i = 0; i < Slices; i++)
		{
			for (int j = 0; j < Slices; j++)
			{
				for (int k = 0; k < Slices; k++)
				{
					SceneNode* pCubeSceneNode = pAssetStandard->CreateSceneNode(pScene);
					pCubeSceneNode->Position = glm::vec3(i * Offset - halfOffset, j * Offset - halfOffset, k * Offset - halfOffset);
					//pCubeSceneNode->Position.z += ZStart;
					//pCubeSceneNode->Position.x += XStart;
					pCubeSceneNode->Scale = glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f));
					pCubeSceneNode->Orientation.Angles = glm::linearRand(glm::vec3(0.0f), glm::vec3(360.0f));
					TEST_NODES.push_back(pCubeSceneNode);
				}
			}
		}

		//pScene->Initialize();
		sceneMgr.SetActiveScene(pScene->GetName());


		//String strAsset = "F:/Models/FBX/_Animated_/teddy.fbx";
		//Asset* pAsset = ImportAsset(strAsset, SunEngine::AssetImporter::Options::Default);
		////pAsset->GetRoot()->Scale *= 0.01f;

		String strAsset = "F:/Models/FBX/_Animated_/teddy.fbx";
		strAsset = "F:/Models/Scenes/EmeraldSquare1024/EmeraldSquare.fbx";
		//strAsset = "F:/Downloads/SunTemple_v3/SunTemple/SunTemple.fbx";
		//strAsset = "F:/Models/OBJ/Small_Tropical_Island/Small Tropical Island.obj";
		//strAsset = "F:/Models/FBX/_PHONG_/RuralStallObj/RuralStall_phong.fbx";
		//strAsset = "F:/Models/Scenes/Bistro/Bistro_Research_Interior.fbx";
		//strAsset = "F:/Downloads/Bistro/Bistro_Research_Exterior.fbx";
		//strAsset = "F:/Models/Scenes/sponza/sponza.obj";
		//strAsset = "F:/Models/Scenes/sibenik/sibenik.obj";
		//strAsset = "F:/Models/FBX/_PBR_/MP44_fbx/MP44/MP44.FBX";

		//auto options = SunEngine::AssetImporter::Options::Default;
		//options.MaxTextureSize = 1024;
		//Asset* pAsset = ImportAsset(strAsset, options);
		//pAsset->CreateSceneNode(pScene, 200.0f);

		return true;
	}
}

