#include <spdlog/spdlog.h>
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
#include "Terrain.h"
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
			viewInfo.visible = true;
			viewInfo.floatingPointColorBuffer = false;
		}

		SceneView* pView = new SceneView(&_sceneRenderer);
		//_view->SetRenderToGraphicsWindow(true);
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

		View::CreateInfo debugViewInfo = {};
		debugViewInfo.floatingPointColorBuffer = false;
		debugViewInfo.visible = false;
		debugViewInfo.height = 256;
		debugViewInfo.width = 256;

		auto pShadowView = new ShadowMapView(&_sceneRenderer);
		pShadowView->Create(debugViewInfo);
		AddView(pShadowView);

		auto pTexture2DView = new Texture2DView();
		pTexture2DView->Create(debugViewInfo);
		AddView(pTexture2DView);

		GameEditorGUI* gui = new GameEditorGUI();
		gui->RegisterSceneView(pView);
		*ppOutGUI = gui;
		spdlog::info("GameEditor initialization succesful");
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
		//for (SceneNode* node : TEST_NODES) node->Orientation.Angles.y += 5.0f;

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
			
			//SceneNode* pSceneNode = pAssetStandard->CreateSceneNode(pScene);
			//pSceneNode->Position = glm::vec3(3.0f, 1.0f, 3.0f);
			//pSceneNode->Orientation.Angles = glm::vec3(144, 223, 298);
		}

		Asset* pAssetBlinnPhong = resMgr.AddAsset("AssetBlinnPhong");
		{
			AssetNode* pRoot = pAssetBlinnPhong->AddNode("Sphere");
			MeshRenderer* pRenderer = pRoot->AddComponent(new MeshRenderer())->As<MeshRenderer>();
			pRenderer->SetMesh(resMgr.GetMesh(DefaultResource::Mesh::Sphere));
			pRenderer->SetMaterial(resMgr.Clone(pSpecularMaterial));
			pRenderer->GetMaterial()->RegisterToGPU();

			//SceneNode* pSceneNode = pAssetBlinnPhong->CreateSceneNode(pScene);
			//pSceneNode->Position = glm::vec3(+0.0f, 1.5f, 0.0f);
			//pSceneNode->Scale = glm::vec3(30, 0.01f, 30.0f);
		}

		float testWorldSize = 120;
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
			pSceneNode->Position = glm::vec3(0.0f, -2.0f, 0.0f);
			pSceneNode->Scale = glm::vec3(testWorldSize, 1.0f, testWorldSize);
		}

		int Slices = 0*1;
		float Offset = 3.0f;
		float ZStart = -(Slices * Slices * 2.0f);
		float XStart = -(Slices * Slices * 0.5f);

		float halfOffset = (Slices / 2) * Offset;

		Asset* pAssetTestCubes = resMgr.AddAsset("AssetTestCubes");
		{
			AssetNode* pRoot = pAssetTestCubes->AddNode("Root");
			for (int i = 0; i < Slices; i++)
			{
				for (int j = 0; j < Slices; j++)
				{
					AssetNode* pCube = pAssetTestCubes->AddNode(StrFormat("Cube%d%d", i, j));
					pAssetTestCubes->SetParent(pCube->GetName(), pRoot->GetName());
					MeshRenderer* pRenderer = pCube->AddComponent(new MeshRenderer())->As<MeshRenderer>();
					pRenderer->SetMesh(resMgr.GetMesh(DefaultResource::Mesh::Cube));
					pRenderer->SetMaterial(resMgr.Clone(pMetalMaterial));
					pRenderer->GetMaterial()->RegisterToGPU();
					pRenderer->GetMaterial()->SetMaterialVar(MaterialStrings::DiffuseColor, glm::linearRand(Vec3::Zero, Vec3::One));

					pCube->Position = glm::vec3(i * Offset - halfOffset, -1.5f, j * Offset - halfOffset);
					pCube->Position = glm::linearRand(glm::vec3(-testWorldSize, 0.0f, -testWorldSize) * 0.5f, glm::vec3(testWorldSize, 0.0f, testWorldSize) * 0.5f);
					pCube->Scale.y = 10;
					//pCubeSceneNode->Position.x += XStart;
					//pCubeSceneNode->Scale = glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f));
					pCube->Orientation.Angles.x = glm::linearRand((0.0f), (360.0f));
				}
			}
			SceneNode* pNode =  pAssetTestCubes->CreateSceneNode(pScene);
		}

		Asset* pAssetTerrain = resMgr.AddAsset("Terrain");
		if(false)
		{
			AssetNode* pRoot = pAssetTerrain->AddNode("Terrain");
			Terrain* pTerrain = pRoot->AddComponent(new Terrain())->As<Terrain>();
			SceneNode* pTerrainNode = pAssetTerrain->CreateSceneNode(pScene);

			Texture2D* biomeTex = resMgr.AddTexture2D("omg");
			biomeTex->SetFilename("C:/Users/Bart/Documents/World Machine Documents/New Project Height Output-1025.r16");
			biomeTex->LoadFromRAW16();
			auto* biome = pTerrain->AddBiome("Test");
			biome->SetTexture(biomeTex);

			//Texture2DArray* pTerarinTexArray = resMgr.AddTexture2DArray("TerrainTextures");
			//pTerarinTexArray->SetWidth(512);
			//pTerarinTexArray->SetHeight(512);

			String basePath = "F:/Models/TerrainTextures/terrain/";

			Vector<String> grassTextureFilenames =
			{
				"grass_rocky_d.jpg",
				"grass_mix_d.jpg",
				"grass_ground_d.jpg",
				"grass_green_d.jpg",
				"desert_mntn_d.jpg",
			};
			Vector<Texture2D*> grassDiffuseTextures;
			Vector<Texture2D*> grassNormalTextures;

			Pair<String, String> diffuseToNormal = { "_d", "_n" };

			for (uint i = 0; i < grassTextureFilenames.size(); i++)
			{
				Texture2D* pTex = resMgr.AddTexture2D(grassTextureFilenames[i]);
				pTex->SetFilename(basePath + grassTextureFilenames[i]);
				if (!pTex->LoadFromFile())
					return false;
				grassDiffuseTextures.push_back(pTex);
				
				String normalFilename = grassTextureFilenames[i];
				normalFilename = normalFilename.replace(normalFilename.find(diffuseToNormal.first), diffuseToNormal.first.length(), diffuseToNormal.second);
				pTex = resMgr.AddTexture2D(normalFilename);
				pTex->SetFilename(basePath + normalFilename);
				if (!pTex->LoadFromFile())
				{
					resMgr.Remove(pTex);
					pTex = resMgr.GetTexture2D(DefaultResource::Texture::Normal);
				}
				grassNormalTextures.push_back(pTex);
			}

			if (!pTerrain->SetDiffuseMap(0, grassDiffuseTextures[0]))
				return false;
			if (!pTerrain->SetDiffuseMap(1, grassDiffuseTextures[1]))
				return false;
			if (!pTerrain->SetDiffuseMap(2, grassDiffuseTextures[4]))
				return false;
			pTerrain->BuildDiffuseMapArray(true);

			if (!pTerrain->SetNormalMap(0, grassNormalTextures[0]))
				return false;
			if (!pTerrain->SetNormalMap(1, grassNormalTextures[1]))
				return false;
			if (!pTerrain->SetNormalMap(2, grassNormalTextures[4]))
				return false;
			pTerrain->BuildNormalMapArray(true);

			//if (!pTerarinTexArray->GenerateMips(true))
			//	return false;

			//pTerarinTexArray->SetSRGB();
			//if (!pTerarinTexArray->RegisterToGPU())
			//	return false;

			Timer t(true);
			pTerrain->UpdateBiomes();
			double et = t.Tick();
			spdlog::info("UpdateBiomes {}", et);
		

		}

		//pScene->Initialize();
		sceneMgr.SetActiveScene(pScene->GetName());


		//String strAsset = "F:/Models/FBX/_Animated_/teddy.fbx";
		//Asset* pAsset = ImportAsset(strAsset, SunEngine::AssetImporter::Options::Default);
		////pAsset->GetRoot()->Scale *= 0.01f;

		String strAsset = "F:/Models/FBX/_Animated_/shark/Silvertip+Shark.fbx";
		strAsset = "F:/Models/FBX/_Animated_/shark/Silvertip+Shark.fbx";
		//strAsset = "F:/Models/FBX/_Animated_/new/48-cat_rigged/cat_rigged.fbx";
		strAsset = "F:/Models/FBX/_Animated_/81881_Elf__game-ready_and_animated/Elf-Final.fbx";
		//strAsset = "F:/GraphicsSamples/ogldev-source/Content/boblampclean.md5mesh";
		strAsset = "F:/Models/Scenes/EmeraldSquare1024/EmeraldSquare.fbx";
		//strAsset = "F:/Downloads/SunTemple_v3/SunTemple/SunTemple.fbx";
		//strAsset = "F:/Models/OBJ/Small_Tropical_Island/Small Tropical Island.obj";
		//strAsset = "F:/Models/FBX/_PHONG_/RuralStallObj/RuralStall_phong.fbx";
		//strAsset = "F:/Models/Scenes/Bistro/Bistro_Research_Interior.fbx";
		//strAsset = "F:/Models/Scenes/Bistro/Bistro_Research_Exterior.fbx";
		//strAsset = "F:/Models/Scenes/sponza/sponza.obj";
		//strAsset = "F:/Models/Scenes/sibenik/sibenik.obj";
		//strAsset = "F:/Models/FBX/_PBR_/MP44_fbx/MP44/MP44.FBX";
		//strAsset = "F:/Models/FBX/_PHONG_/Wooden_barrels__OBJ/Wooden_barrels.FBX";
		//strAsset = "F:/Models/glTF-Sample-Models-master/2.0/DamagedHelmet/glTF/DamagedHelmet.gltf";
		//strAsset = "F:/Code/Vulkan-master/data/models/cerberus/cerberus.gltf";

		auto options = SunEngine::AssetImporter::Options::Default;
		options.MaxTextureSize = 4096;
		Asset* pAsset = 0;
		pAsset = ImportAsset(strAsset, options);
		if (pAsset)
		{
			//auto node = pAsset->GetNodeByName("Cerberus00_Fixed.001");
			//auto renderer = node->GetComponentOfType(COMPONENT_RENDER_OBJECT)->As<MeshRenderer>();

			//Material* pMaterial = resMgr.Clone(pMetalMaterial);
			//renderer->SetMaterial(pMaterial);
			//pMaterial->RegisterToGPU();

			//Texture2D* pTex = 0;
			//String dir = GetDirectory(strAsset) + "/";

			//pTex = resMgr.AddTexture2D("albedo");
			//pTex->SetFilename(dir + "albedo.jpg");
			//pTex->LoadFromFile();
			//pTex->SetSRGB();
			//pTex->RegisterToGPU();
			//pMaterial->SetTexture2D(MaterialStrings::DiffuseMap, pTex);

			//pTex = resMgr.AddTexture2D("normal");
			//pTex->SetFilename(dir + "normal.jpg");
			//pTex->LoadFromFile();
			//pTex->RegisterToGPU();
			//pMaterial->SetTexture2D(MaterialStrings::NormalMap, pTex);

			//pTex = resMgr.AddTexture2D("ao");
			//pTex->SetFilename(dir + "ao.jpg");
			//pTex->LoadFromFile();
			//pTex->RegisterToGPU();
			//pMaterial->SetTexture2D(MaterialStrings::AmbientOcclusionMap, pTex);

			//pTex = resMgr.AddTexture2D("metallic");
			//pTex->SetFilename(dir + "metallic.jpg");
			//pTex->LoadFromFile();
			//pTex->RegisterToGPU();
			//pMaterial->SetTexture2D(MaterialStrings::MetallicMap, pTex);

			//pTex = resMgr.AddTexture2D("roughness");
			//pTex->SetFilename(dir + "roughness.jpg");
			//pTex->LoadFromFile();
			//pTex->RegisterToGPU();
			//pMaterial->SetTexture2D(MaterialStrings::RoughnessMap, pTex);

			pAsset->CreateSceneNode(pScene, 400);
		}
		else
			spdlog::warn("Failed to load {}", strAsset);

		return true;
	}
}

