#include "StringUtil.h"
#include "ResourceMgr.h"
#include "MeshRenderer.h"
#include "Shader.h"
#include "glm/gtx/matrix_decompose.hpp"

#include "AssetImporter.h"

#define COMPARE_MTL_VAR(left, right, Var) if(!(left->Var == right->Var)) return false;

namespace SunEngine
{
	bool ImportedMaterialsSame(ModelImporter::Importer::Material* lhs, ModelImporter::Importer::Material* rhs)
	{
		COMPARE_MTL_VAR(lhs, rhs, DiffuseColor);
		COMPARE_MTL_VAR(lhs, rhs, Alpha);
		COMPARE_MTL_VAR(lhs, rhs, SpecularColor);
		COMPARE_MTL_VAR(lhs, rhs, SpecularExponent);
		COMPARE_MTL_VAR(lhs, rhs, DiffuseMap);
		COMPARE_MTL_VAR(lhs, rhs, NormalMap);
		COMPARE_MTL_VAR(lhs, rhs, SpecularMap);
		COMPARE_MTL_VAR(lhs, rhs, TransparentMap);
		COMPARE_MTL_VAR(lhs, rhs, AmbientMap);
		COMPARE_MTL_VAR(lhs, rhs, SpecularFactorMap);
		COMPARE_MTL_VAR(lhs, rhs, ShininessExponentMap);
		COMPARE_MTL_VAR(lhs, rhs, BumpMap);

		return true;
	}

	AssetImporter::Options MakeDefaultImporterOptions()
	{
		AssetImporter::Options opt;
		opt.CombineMaterials = false;

		return opt;
	}

	const AssetImporter::Options AssetImporter::Options::Default = MakeDefaultImporterOptions();

	AssetImporter::AssetImporter()
	{
		_asset = 0;
	}

	AssetImporter::~AssetImporter()
	{
	}

	bool AssetImporter::Import(const String& filename, const Options& options)
	{
		String ext = StrToLower(GetExtension(filename));

		ModelImporter::Importer* pImporter = ModelImporter::Importer::Create(filename);
		if (!pImporter)
			return false;

		if (!pImporter->Import())
			return false;

		auto& resMgr = ResourceMgr::Get();

		_asset = resMgr.AddAsset(GetFileNameNoExt(filename));
		if (!_asset)
			return false;

		for (uint m = 0; m < pImporter->GetMeshDataCount(); m++)
		{
			auto iMesh = pImporter->GetMeshData(m);
			auto pMesh = resMgr.AddMesh(iMesh->Name);

			pMesh->AllocVertices(iMesh->Vertices.size(), VertexDef::POS_TEXCOORD_NORMAL_TANGENT);
			pMesh->AllocIndices(iMesh->Indices.size());

			auto& def = pMesh->GetVertexDef();

			for (uint i = 0; i < iMesh->Vertices.size(); i++)
			{
				auto& vtx = iMesh->Vertices[i];
				pMesh->SetVertexVar(i, glm::vec4(vtx.position.x, vtx.position.y, vtx.position.z, 1.0f), 0);
				pMesh->SetVertexVar(i, glm::vec4(vtx.texCoord.x, vtx.texCoord.y, 0.0f, 0.0f), def.TexCoordIndex);
				pMesh->SetVertexVar(i, glm::vec4(vtx.normal.x, vtx.normal.y, vtx.normal.z, 0.0f), def.NormalIndex);
				pMesh->SetVertexVar(i, glm::vec4(vtx.tangent.x, vtx.tangent.y, vtx.tangent.z, 0.0f), def.TangentIndex);
			}

			for (uint i = 0; i < iMesh->Indices.size(); i += 3)
			{
				pMesh->SetTri(i / 3, iMesh->Indices[i + 0], iMesh->Indices[i + 1], iMesh->Indices[i + 2]);
			}

			pMesh->UpdateBoundingVolume();

			if (!pMesh->RegisterToGPU())
				return false;

			_meshFixup[iMesh] = pMesh;
		}

		Vector<ModelImporter::Importer::Material*> uniqueMaterials;

		for (uint m = 0; m < pImporter->GetMaterialCount(); m++)
		{
			auto iMtl = pImporter->GetMaterial(m);

			if (options.CombineMaterials)
			{
				bool foundMaterial = false;
				for (uint i = 0; i < uniqueMaterials.size(); i++)
				{
					if (ImportedMaterialsSame(iMtl, uniqueMaterials[i]))
					{
						_materialFixup[iMtl] = _materialFixup.at(uniqueMaterials[i]);
						foundMaterial = true;
						break;
					}
				}

				if (foundMaterial)
					continue;

				uniqueMaterials.push_back(iMtl);
			}

			auto pMtl = resMgr.AddMaterial(iMtl->Name);

			String strShader;
			StrMap<Texture2D*> textures;
			PickMaterialShader(iMtl, strShader, textures);
			pMtl->SetShader(resMgr.GetShader(strShader));

			if (!pMtl->RegisterToGPU())
				return false;

			pMtl->GetShader()->SetDefaults(pMtl);

			for (auto iter = textures.begin(); iter != textures.end(); ++iter)
			{
				pMtl->SetTexture2D((*iter).first, (*iter).second);
			}

			//TODO: make better...
			_materialFixup[iMtl] = pMtl;
		}

		bool needsRoot = pImporter->GetNodeCount() > 1 && (pImporter->GetNode(0)->Parent == 0 && pImporter->GetNode(1)->Parent == 0);
		if (needsRoot)
		{
			_nodeFixup[0] = _asset->AddNode("RootNode");
		}
		else
		{
			_nodeFixup[0] = 0;
		}

		for (uint i = 0; i < pImporter->GetNodeCount(); i++)
		{
			auto* iNode = pImporter->GetNode(i);
			auto pNode = _asset->AddNode(iNode->Name);
			_nodes.push_back(pNode);

			glm::mat4 mtxLocal;
			memcpy(&mtxLocal, &iNode->LocalMatrix, sizeof(glm::mat4));

			glm::vec3 skew;
			glm::vec4 perspective;
			if (glm::decompose(mtxLocal, pNode->Scale, pNode->Orientation.Quat, pNode->Position, skew, perspective))
			{
				pNode->Orientation.Quat = glm::conjugate(pNode->Orientation.Quat);
				pNode->Orientation.Mode = ORIENT_QUAT;
			}

			if (iNode->GetType() == ModelImporter::Importer::MESH)
			{
				auto iMesh = static_cast<ModelImporter::Importer::Mesh*>(iNode);
				glm::mat4 mtxGeom;
				memcpy(&mtxLocal, &iNode->GeometryOffset, sizeof(glm::mat4));

				MeshRenderer* pRenderer = 0;
				if (mtxGeom == glm::mat4(1.0f))
				{
					pRenderer = pNode->AddComponent(new MeshRenderer())->As<MeshRenderer>();
				}
				else
				{
					//Fix Geom Offset existence by creating a mesh renderer node for it
					//Cant bake it into the node because it might have children that should not inherit it

					auto pGeomNode = _asset->AddNode(iNode->Name + "MeshGeom");
					_nodes.push_back(pGeomNode);
					if (glm::decompose(mtxGeom, pGeomNode->Scale, pGeomNode->Orientation.Quat, pGeomNode->Position, skew, perspective))
					{
						pGeomNode->Orientation.Quat = glm::conjugate(pGeomNode->Orientation.Quat);
						pGeomNode->Orientation.Mode = ORIENT_QUAT;
					}
					_asset->SetParent(pGeomNode->GetName(), pNode->GetName());

					pRenderer = pGeomNode->AddComponent(new MeshRenderer())->As<MeshRenderer>();
				}

				pRenderer->SetMesh(_meshFixup.at(iMesh->MeshData));
				pRenderer->SetMaterial(iMesh->Material ? _materialFixup.at(iMesh->Material) : ResourceMgr::Get().GetMaterial(DefaultResource::Material::StandardSpecular));
			}

			auto* parent = _nodeFixup.at(iNode->Parent);
			if (parent)
			{
				_asset->SetParent(pNode->GetName(), parent->GetName());
			}

			_nodeFixup[iNode] = pNode;
		}

		_asset->UpdateBoundingVolume();

		return true;
	}

	bool AssetImporter::PickMaterialShader(void* iMaterial, String& shader, StrMap<Texture2D*>& textures)
	{
		ModelImporter::Importer::Material* pMtl = (ModelImporter::Importer::Material*)iMaterial;

		String strRoughness = pMtl->ShininessExponentMap ? StrToLower(GetFileName(pMtl->ShininessExponentMap->FileName)) : "";

		StrMap<ModelImporter::Importer::Texture*> importerTextures;

		if (StrContains(strRoughness, "gloss") || StrContains(strRoughness, "rough"))
		{
			shader = DefaultResource::Shader::StandardMetallic;

			importerTextures[MaterialStrings::SmoothnessMap] = pMtl->ShininessExponentMap;

			if (pMtl->DiffuseMap) importerTextures[MaterialStrings::DiffuseMap] = pMtl->DiffuseMap;

			if (pMtl->SpecularFactorMap) importerTextures[MaterialStrings::MetalMap] = pMtl->SpecularFactorMap;
			else if (pMtl->SpecularMap) importerTextures[MaterialStrings::MetalMap] = pMtl->SpecularMap;

			if (pMtl->NormalMap) importerTextures[MaterialStrings::NormalMap] = pMtl->NormalMap;
			else if (pMtl->BumpMap) importerTextures[MaterialStrings::NormalMap] = pMtl->BumpMap;

			if (pMtl->AmbientMap) importerTextures[MaterialStrings::AmbientOcclusionMap] = pMtl->AmbientMap;
		}
		else
		{
			shader = DefaultResource::Shader::StandardSpecular;

			if (pMtl->DiffuseMap) importerTextures[MaterialStrings::DiffuseMap] = pMtl->DiffuseMap;

			if (pMtl->SpecularFactorMap) importerTextures[MaterialStrings::SpecularMap] = pMtl->SpecularFactorMap;
			else if (pMtl->SpecularMap) importerTextures[MaterialStrings::SpecularMap] = pMtl->SpecularMap;
			else if(pMtl->ShininessExponentMap) importerTextures[MaterialStrings::SpecularMap] = pMtl->ShininessExponentMap;

			if (pMtl->NormalMap) importerTextures[MaterialStrings::NormalMap] = pMtl->NormalMap;
			else if (pMtl->BumpMap) importerTextures[MaterialStrings::NormalMap] = pMtl->BumpMap;
		}

		for (auto iter = importerTextures.begin(); iter != importerTextures.end(); ++iter)
		{
			Texture2D* pTex = 0;
			auto texIter = ResourceMgr::Get().IterTextures2D();

			String filename = StrToLower((*iter).second->FileName);

			while (!texIter.End())
			{
				if (StrToLower((*texIter)->GetFilename()) == filename)
				{
					pTex = *texIter;
					break;
				}
				++texIter;
			}

			if (pTex == 0)
			{
				pTex = ResourceMgr::Get().AddTexture2D(GetFileName(filename));
				if (!pTex->LoadFromFile((*iter).second->FileName))
					return false;

				if (!pTex->GenerateMips())
					return false;

				if ((*iter).first == MaterialStrings::DiffuseMap)
					pTex->SetSRGB(true);

				if (!pTex->RegisterToGPU())
					return false;
			}

			textures[(*iter).first] = pTex;
		}

		return true;
	}
}