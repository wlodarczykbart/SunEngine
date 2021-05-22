#if 0

#include "StringUtil.h"
#include "ResourceMgr.h"
#include "MeshRenderer.h"
#include "ShaderMgr.h"
#include "Animation.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "ThreadPool.h"
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
		opt.MaxTextureSize = 4096;

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
		_options = options;
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

		//Map<uint, float> inUseBones;

		for (uint m = 0; m < pImporter->GetMeshDataCount(); m++)
		{
			auto iMesh = pImporter->GetMeshData(m);
			auto pMesh = resMgr.AddMesh(iMesh->Name);

			VertexDef vtxDef = VertexDef::POS_TEXCOORD_NORMAL_TANGENT;
			Vector<Pair<float, float>> sortedBones;
			uint skinnedVertexDataStartAttrib = 0;
			if (iMesh->VertexBones.size())
			{
				vtxDef.NumVars += 2;
				skinnedVertexDataStartAttrib = vtxDef.TangentIndex;
				sortedBones.resize(ModelImporter::Importer::VertexBoneInfo::MAX_VERTEX_BONES);
			}

			pMesh->AllocVertices(iMesh->Vertices.size(), vtxDef);
			pMesh->AllocIndices(iMesh->Indices.size());

			for (uint i = 0; i < iMesh->Vertices.size(); i++)
			{
				auto& vtx = iMesh->Vertices[i];
				pMesh->SetVertexVar(i, glm::vec4(vtx.position.x, vtx.position.y, vtx.position.z, 1.0f), 0);
				pMesh->SetVertexVar(i, glm::vec4(vtx.texCoord.x, vtx.texCoord.y, 0.0f, 0.0f), vtxDef.TexCoordIndex);
				pMesh->SetVertexVar(i, glm::vec4(vtx.normal.x, vtx.normal.y, vtx.normal.z, 0.0f), vtxDef.NormalIndex);
				pMesh->SetVertexVar(i, glm::vec4(vtx.tangent.x, vtx.tangent.y, vtx.tangent.z, 0.0f), vtxDef.TangentIndex);

				if (iMesh->VertexBones.size())
				{
					for (uint b = 0; b < sortedBones.size(); b++)
						sortedBones[b] = { iMesh->VertexBones[i].bones[b], iMesh->VertexBones[i].weights[b] };

					std::sort(sortedBones.begin(), sortedBones.end(), [] (const Pair<float, float>& left, const Pair<float, float>& right) -> bool { return left.second > right.second; });

					float sum = 0.0f;
					for (uint b = 0; b < 4; b++)
						sum += sortedBones[b].second;

					glm::vec4 bones;
					glm::vec4 weights;
					for (uint b = 0; b < 4; b++)
					{
						bones[b] = sortedBones[b].first;
						weights[b] = sortedBones[b].second / sum; //renormalize around 4 max weights

						//inUseBones[(uint)bones[b]] = glm::max(inUseBones[(uint)bones[b]], weights[b]);
					}

					float sum1 = glm::dot(weights, glm::vec4(1.0f));
					assert(glm::epsilonEqual(sum1, 1.0f, 0.00001f));

					pMesh->SetVertexVar(i, bones, skinnedVertexDataStartAttrib + 1);
					pMesh->SetVertexVar(i, weights, skinnedVertexDataStartAttrib + 2);
				}
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

		Animator* pAnimator = 0;
		Vector<Vector<Vector<AnimatedBone::Transform>>> boneTransforms;
		Map<ModelImporter::Importer::Bone*, uint> boneIndexLookup;

		if (pImporter->GetAnimationCount())
		{
			pAnimator = new Animator();
			pAnimator->SetBoneCount(pImporter->GetBoneCount());

			boneTransforms.resize(pImporter->GetBoneCount());
			for (uint b = 0; b < pImporter->GetBoneCount(); b++)
			{
				boneIndexLookup[pImporter->GetBone(b)] = b;
				boneTransforms[b].resize(pImporter->GetAnimationCount());
			}

			Vector<AnimationClip> clips;
			clips.resize(pImporter->GetAnimationCount());
			for (uint a = 0; a < pImporter->GetAnimationCount(); a++)
			{
				AnimationClip& clip = clips[a];

				auto iAnim = pImporter->GetAnimation(a);

				for (uint b = 0; b < pImporter->GetBoneCount(); b++)
					boneTransforms[b][a].resize(iAnim->KeyFrames.size());

				Vector<float> keys;
				keys.resize(iAnim->KeyFrames.size());
				for (uint k = 0; k < keys.size(); k++)
				{
					auto& iKey = iAnim->KeyFrames[k];
					keys[k] = (float)iKey.Time; //TODO: use doubles?

					for (uint b = 0; b < iKey.Transforms.size(); b++)
					{
						auto& iTransform = iKey.Transforms[b];

						AnimatedBone::Transform transform;
						transform.position = glm::vec3(iTransform.Translation.x, iTransform.Translation.y, iTransform.Translation.z);
						transform.scale = glm::vec3(iTransform.Scale.x, iTransform.Scale.y, iTransform.Scale.z);

						Orientation o;
						o.Mode = ORIENT_XYZ;
						o.Angles = glm::degrees(glm::vec3(iTransform.Rotation.x, iTransform.Rotation.y, iTransform.Rotation.z));
						transform.rotation = glm::quat_cast(o.BuildMatrix());
						boneTransforms[b][a][k] = transform;
					}
				}

				clip.SetKeys(keys);
				clip.SetName(iAnim->Name);
			}
			pAnimator->SetClips(clips);
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
				//glm::mat4 rotMtx = glm::toMat4(pNode->Orientation.Quat);
				//glm::extractEulerAngleXYZ(rotMtx, pNode->Orientation.Angles.x, pNode->Orientation.Angles.y, pNode->Orientation.Angles.z);
				//pNode->Orientation.Angles = glm::degrees(pNode->Orientation.Angles);
				//pNode->Orientation.Mode = ORIENT_XYZ;
				pNode->Orientation.Quat = glm::conjugate(pNode->Orientation.Quat);
				pNode->Orientation.Mode = ORIENT_QUAT;
			}

			if (iNode->GetType() == ModelImporter::Importer::MESH)
			{
				auto iMesh = static_cast<ModelImporter::Importer::Mesh*>(iNode);
				glm::mat4 mtxGeom;
				memcpy(&mtxGeom, &iNode->GeometryOffset, sizeof(glm::mat4));

				MeshRenderer* pRenderer = 0;
				AssetNode* pMeshNode = 0;
				if (mtxGeom == glm::mat4(1.0f))
				{
					pRenderer = pNode->AddComponent(new MeshRenderer())->As<MeshRenderer>();
					pMeshNode = pNode;
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
					pMeshNode = pGeomNode;
				}

				pRenderer->SetMesh(_meshFixup.at(iMesh->MeshData));
				Material* pMtl = 0;
				if (!ChooseMaterial(iMesh, pMtl))
					return false;
				pRenderer->SetMaterial(pMtl);

				if (iMesh->MeshData->VertexBones.size())
				{
					SkinnedMesh* pSkinned = pMeshNode->AddComponent(new SkinnedMesh())->As<SkinnedMesh>();
					pSkinned->SetSkinIndex(iMesh->MeshData->SkinIndex);
				}
			}
			else if (iNode->GetType() == ModelImporter::Importer::BONE)
			{
				auto iBone = static_cast<ModelImporter::Importer::Bone*>(iNode);
				
				AnimatedBone* pBone = pNode->AddComponent(new AnimatedBone())->As<AnimatedBone>();
				Vector<glm::mat4> skinMatrices;
				skinMatrices.resize(iBone->SkinMatrices.size());
				memcpy(skinMatrices.data(), iBone->SkinMatrices.data(), sizeof(glm::mat4) * skinMatrices.size());
				uint boneIndex = boneIndexLookup.at(iBone);
				pBone->SetBoneIndex(boneIndex);
				pBone->SetSkinMatrices(skinMatrices);
				auto& transforms = boneTransforms[boneIndex];
				pBone->SetTransforms(transforms);
			}

			auto* parent = _nodeFixup.at(iNode->Parent);
			if (parent)
			{
				_asset->SetParent(pNode->GetName(), parent->GetName());
			}

			_nodeFixup[iNode] = pNode;
		}

		//textures were added when materials were parsed
		ThreadPool& tp = ThreadPool::Get();
		tp.Wait();

		for (auto pTexture : _textureLoadList)
		{
			if (!pTexture.first->RegisterToGPU())
				return false;
		}

		for (auto& mtl : _materialCache)
		{
			auto& textures = mtl.second.second;
			for (auto& tex : textures)
			{
				mtl.first->SetTexture2D(tex.first, tex.second);
			}
		}

		if (pAnimator)
		{
			_asset->GetRoot()->AddComponent(pAnimator);
		}
		return true;
	}

	bool AssetImporter::ChooseMaterial(void* iMesh, Material*& pOutMtl)
	{
		ModelImporter::Importer::Mesh* pMesh = (ModelImporter::Importer::Mesh*)iMesh;
		ModelImporter::Importer::Material* pMtl = pMesh->Material;

		if (pMtl == 0)
		{
			pOutMtl = ResourceMgr::Get().GetMaterial(DefaultResource::Material::StandardSpecular);
			return true;
		}

		String strRoughness = pMtl->ShininessExponentMap ? StrToLower(GetFileName(pMtl->ShininessExponentMap->FileName)) : "";

		StrMap<ModelImporter::Importer::Texture*> importerTextures;

		String shader;
		StrMap<Texture2D*> textures;

		if (StrContains(strRoughness, "gloss") || StrContains(strRoughness, "rough"))
		{
			shader = DefaultShaders::Metallic;

			importerTextures[MaterialStrings::SmoothnessMap] = pMtl->ShininessExponentMap;

			if (pMtl->DiffuseMap) importerTextures[MaterialStrings::DiffuseMap] = pMtl->DiffuseMap;

			if (pMtl->SpecularFactorMap) importerTextures[MaterialStrings::MetalMap] = pMtl->SpecularFactorMap;
			else if (pMtl->SpecularMap) importerTextures[MaterialStrings::MetalMap] = pMtl->SpecularMap;

			if (pMtl->NormalMap) importerTextures[MaterialStrings::NormalMap] = pMtl->NormalMap;
			else if (pMtl->BumpMap) importerTextures[MaterialStrings::NormalMap] = pMtl->BumpMap;

			if (pMtl->AmbientMap) importerTextures[MaterialStrings::AmbientOcclusionMap] = pMtl->AmbientMap;

			if (pMtl->TransparentMap) importerTextures[MaterialStrings::AlphaMap] = pMtl->TransparentMap;
		}
		else
		{
			shader = DefaultShaders::Specular;

			if (pMtl->DiffuseMap) importerTextures[MaterialStrings::DiffuseMap] = pMtl->DiffuseMap;

			if (pMtl->SpecularFactorMap) importerTextures[MaterialStrings::SpecularMap] = pMtl->SpecularFactorMap;
			else if (pMtl->SpecularMap) importerTextures[MaterialStrings::SpecularMap] = pMtl->SpecularMap;
			else if(pMtl->ShininessExponentMap) importerTextures[MaterialStrings::SpecularMap] = pMtl->ShininessExponentMap;

			if (pMtl->NormalMap) importerTextures[MaterialStrings::NormalMap] = pMtl->NormalMap;
			else if (pMtl->BumpMap) importerTextures[MaterialStrings::NormalMap] = pMtl->BumpMap;

			if (pMtl->TransparentMap) importerTextures[MaterialStrings::AlphaMap] = pMtl->TransparentMap;
		}

		//TODO: need skinned/alpha test variant or assume it wont happen?
		if (pMesh->MeshData->VertexBones.size())
		{
			if (shader == DefaultShaders::Metallic) shader = DefaultShaders::SkinnedMetallic;
			else if (shader == DefaultShaders::Specular) shader = DefaultShaders::SkinnedSpecular;
		}
		else if (pMtl->TransparentMap)
		{
			if (shader == DefaultShaders::Metallic) shader = DefaultShaders::MetallicAlphaTest;
			else if (shader == DefaultShaders::Specular) shader = DefaultShaders::SpecularAlphaTest;
		}

		for (auto& cached : _materialCache)
		{
			bool equalShader = shader == cached.first->GetShader()->GetName();
			bool equalMaterial = _options.CombineMaterials ? (cached.second.first == pMtl || ImportedMaterialsSame(pMtl, (ModelImporter::Importer::Material*)cached.second.first)) : cached.second.first == pMtl;
			if (equalShader && equalMaterial)
			{
				pOutMtl = cached.first;
				return true;
			}
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
				pTex->SetFilename((*iter).second->FileName);
				pTex->SetUserDataPtr(this);

				if ((*iter).first == MaterialStrings::DiffuseMap)
					pTex->SetSRGB(true);

				ThreadPool& tp = ThreadPool::Get();
				tp.AddTask([](uint, void* pData) -> void {
					Texture2D* pTexture = static_cast<Texture2D*>(pData);
					AssetImporter* pThis = static_cast<AssetImporter*>(pTexture->GetUserDataPtr());
					if (pTexture->LoadFromFile())
					{
						uint maxSize = pThis->_options.MaxTextureSize;
						if (pTexture->GetWidth() > maxSize || pTexture->GetHeight() > maxSize) 
						{
							pTexture->Resize(glm::min(pTexture->GetWidth(), maxSize), glm::min(pTexture->GetHeight(), maxSize));
						}
						pTexture->GenerateMips(false);

						if((*pThis->_textureLoadList.find(pTexture)).second != MaterialStrings::NormalMap)
							pTexture->Compress();
					}
				}, pTex);

				_textureLoadList[pTex] = (*iter).first;
			}

			textures[(*iter).first] = pTex;
		}

		pOutMtl = ResourceMgr::Get().AddMaterial(pMtl->Name);
		pOutMtl->SetShader(ShaderMgr::Get().GetShader(shader));

		if (!pOutMtl->RegisterToGPU())
			return false;

		pOutMtl->GetShader()->SetDefaults(pOutMtl);
		_materialCache[pOutMtl] = { pMtl, textures };
		return true;
	}
}

#else

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <filesystem>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "StringUtil.h"
#include "ResourceMgr.h"
#include "MeshRenderer.h"
#include "ShaderMgr.h"
#include "Animation.h"
#include "FileBase.h"
#include "ThreadPool.h"


#include "AssetImporter.h"

namespace SunEngine
{
	AssetImporter::Options MakeDefaultImporterOptions()
	{
		AssetImporter::Options opt;
		opt.CombineMaterials = false;
		opt.MaxTextureSize = 4096;

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

	glm::vec3 FromAssimp(const aiVector3D& v)
	{
		return glm::vec3(v.x, v.y, v.z);
	}

	void CollectNodes(aiNode* pNode, Vector<aiNode*>& nodes)
	{
		nodes.push_back(pNode);

		for (uint i = 0; i < pNode->mNumChildren; i++)
			CollectNodes(pNode->mChildren[i], nodes);
	}

	bool ParseMesh(aiMesh* pSrc, Mesh* pDst)
	{
		pDst->AllocIndices(pSrc->mNumFaces * 3);
		pDst->AllocVertices(pSrc->mNumVertices, VertexDef::POS_TEXCOORD_NORMAL_TANGENT);

		for (uint i = 0; i < pSrc->mNumVertices; i++)
		{
			pDst->SetVertexVar(i, glm::vec4(FromAssimp(pSrc->mVertices[i]), 1.0f));
			if (pSrc->HasTextureCoords(0))
			{
				glm::vec4 uv = glm::vec4(FromAssimp(pSrc->mTextureCoords[0][i]), 0.0f);
				uv.y = 1.0f - uv.y;
				pDst->SetVertexVar(i, uv, VertexDef::DEFAULT_TEX_COORD_INDEX);
			}
			if(pSrc->HasNormals())
				pDst->SetVertexVar(i, glm::vec4(FromAssimp(pSrc->mNormals[i]), 0.0f), VertexDef::DEFAULT_NORMAL_INDEX);
			if(pSrc->HasTangentsAndBitangents())
				pDst->SetVertexVar(i, glm::vec4(FromAssimp(pSrc->mTangents[i]), 0.0f), VertexDef::DEFAULT_TANGENT_INDEX);
		}

		for (uint i = 0; i < pSrc->mNumFaces; i++)
		{
			auto& tri = pSrc->mFaces[i];
			pDst->SetTri(i, tri.mIndices[0], tri.mIndices[1], tri.mIndices[2]);
		}

		pDst->UpdateBoundingVolume();

		if (!pDst->RegisterToGPU())
			return false;

		return true;
	}

	bool ParseMaterial(aiMaterial* pSrc, Material* pDst, StrMap<Pair<Texture2D*, bool>>& textures, const String& fileDir)
	{
		static const Map<aiTextureType, String> TextureTypeLookup =
		{
			{ aiTextureType_DIFFUSE, MaterialStrings::DiffuseMap },
			{ aiTextureType_SPECULAR, MaterialStrings::SpecularMap },
			{ aiTextureType_NORMALS, MaterialStrings::NormalMap },
			{ aiTextureType_OPACITY, MaterialStrings::AlphaMap },
			{ aiTextureType_SHININESS, MaterialStrings::SmoothnessMap },
			{ aiTextureType_BASE_COLOR, MaterialStrings::DiffuseMap },
			{ aiTextureType_METALNESS, MaterialStrings::MetalMap },
			{ aiTextureType_DIFFUSE_ROUGHNESS, MaterialStrings::Smoothness },
			{ aiTextureType_AMBIENT_OCCLUSION, MaterialStrings::AmbientOcclusionMap },
			{ aiTextureType_LIGHTMAP, MaterialStrings::AmbientOcclusionMap },
		};

		StrMap<aiTextureType> allTextures;
		for (uint i = aiTextureType_NONE + 1; i < aiTextureType_UNKNOWN; i++)
		{
			uint texCount = pSrc->GetTextureCount(aiTextureType(i));
			for (uint j = 0; j < texCount; j++)
			{
				aiString path;
				if (pSrc->GetTexture((aiTextureType)i, j, &path) == aiReturn_SUCCESS)
					allTextures[path.C_Str()] = (aiTextureType)i;
			}
		}

		Map<aiTextureType, bool> usedTextureTypes;
		for (auto& texType : TextureTypeLookup)
		{
			uint texCount = pSrc->GetTextureCount(texType.first);
			if (texCount)
			{
				String strPath;

				aiString path;
				pSrc->GetTexture(texType.first, 0, &path);
				FileStream fs;
				if (fs.OpenForRead(path.C_Str())) //easiest case if file path is correct
				{
					strPath = path.C_Str();
					fs.Close();
				}
				else if (fs.OpenForRead((fileDir + path.C_Str()).c_str())) //second easiest case if the file is relative to the input file directory
				{
					strPath = fileDir + path.C_Str();
					fs.Close();
				}
				else //try to find the file somewhere in the input file directory
				{
					String filename = StrToLower(GetFileName(path.C_Str()));
					for (auto& fsiter : std::filesystem::recursive_directory_iterator(fileDir))
					{
						if (fsiter.path().has_filename())
						{
							String currFile = fsiter.path().filename().u8string();
							if (filename == StrToLower(GetFileName(currFile)))
							{
								strPath = fsiter.path().u8string();
								break;
							}
						}
					}
				}

				if (!strPath.empty() && Image::CanLoad(strPath))
				{
					auto& resMgr = ResourceMgr::Get();
					Texture2D* pTexture = resMgr.GetTexture2D(strPath);
					bool needsLoad = false;
					if (!pTexture)
					{
						pTexture = resMgr.AddTexture2D(strPath);
						pTexture->SetFilename(strPath);

						if (texType.second == MaterialStrings::DiffuseMap)
							pTexture->SetSRGB();

						needsLoad = true;
					}

					usedTextureTypes[texType.first] = true;
					textures[texType.second] = { pTexture, needsLoad };
					allTextures.erase(path.C_Str());
				}
			}
		}

		bool metallic =
			usedTextureTypes.find(aiTextureType_DIFFUSE_ROUGHNESS) != usedTextureTypes.end() ||
			usedTextureTypes.find(aiTextureType_METALNESS) != usedTextureTypes.end();

		bool alphaTest = usedTextureTypes.find(aiTextureType_OPACITY) != usedTextureTypes.end();
		alphaTest = false; //TODO: enable this again? default to alpha blend

		String strShader;
		if (metallic)
		{
			strShader = alphaTest ? DefaultShaders::MetallicAlphaTest : DefaultShaders::Metallic;
		}
		else
		{
			strShader = alphaTest ? DefaultShaders::SpecularAlphaTest : DefaultShaders::Specular;
		}

		pDst->SetShader(ShaderMgr::Get().GetShader(strShader));
		if (!pDst->RegisterToGPU())
			return false;

		pDst->GetShader()->SetDefaults(pDst);

		//values that are in shader material buffer
		aiColor4D diffuseColor;
		aiColor4D transparencyFactor;
		aiColor4D transparentColor;

		aiColor4D specularColor;
		aiColor4D smoothness;
		aiColor4D shininessStrength;

		aiColor4D twoSided;
		aiColor4D ambientColor;
		aiColor4D emissiveColor;

		pDst->GetMaterialVar(MaterialStrings::DiffuseColor, diffuseColor);
		pDst->GetMaterialVar(MaterialStrings::SpecularColor, specularColor);
		pDst->GetMaterialVar(MaterialStrings::Smoothness, smoothness);

		pSrc->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
		pSrc->Get(AI_MATKEY_OPACITY, diffuseColor.a);
		//if(pSrc->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparencyFactor) == aiReturn_SUCCESS) diffuseColor.a *= transparencyFactor.r;
		if(pSrc->Get(AI_MATKEY_COLOR_TRANSPARENT, transparentColor) == aiReturn_SUCCESS && transparentColor.a != 0.0f)  
			diffuseColor.a *= transparentColor.r;

		pSrc->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
		pSrc->Get(AI_MATKEY_SHININESS, smoothness);
		pSrc->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength);

		//Not sure if these are of any use to me
		pSrc->Get(AI_MATKEY_TWOSIDED, twoSided);
		pSrc->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);
		pSrc->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);

		//handle phong specualar exponent this way for now...
		if (smoothness.r > 1.0f)
			smoothness.r = 1.0f - expf(-smoothness.r * 0.008f);

		//pDst->SetMaterialVar(MaterialStrings::DiffuseColor, diffuseColor);
		//pDst->SetMaterialVar(MaterialStrings::SpecularColor, specularColor);
		//pDst->SetMaterialVar(MaterialStrings::Smoothness, smoothness);

		return true;
	}

	bool AssetImporter::Import(const String& filename, const Options& options)
	{
		_options = options;

		auto pScene = aiImportFile(filename.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
		if (!pScene)
			return false;

		bool lights = pScene->HasLights();
		bool cameras = pScene->HasCameras();

		auto& resMgr = ResourceMgr::Get();

		_asset = resMgr.AddAsset(GetFileNameNoExt(filename));
		if (!_asset)
			return false;

		Vector<aiNode*> nodes;
		CollectNodes(pScene->mRootNode, nodes);

		String fileDir = GetDirectory(filename) + "/";

		ThreadPool& tp = ThreadPool::Get();

		for (auto aNode : nodes)
		{
			auto pNode = _asset->AddNode(aNode->mName.length ? aNode->mName.C_Str() : StrFormat("%p", aNode));
			_nodeFixup[aNode] = pNode;

			glm::mat4 mtxLocal;
			memcpy(&mtxLocal, &aNode->mTransformation, sizeof(glm::mat4));
			mtxLocal = glm::transpose(mtxLocal);

			glm::vec3 skew;
			glm::vec4 perspective;
			if (glm::decompose(mtxLocal, pNode->Scale, pNode->Orientation.Quat, pNode->Position, skew, perspective))
			{
				if (aNode != pScene->mRootNode)
				{
					pNode->Orientation.Quat = glm::conjugate(pNode->Orientation.Quat);
					pNode->Orientation.Mode = ORIENT_QUAT;
				}
				else
				{
					//Allow root node to be rotated from eular angles
					glm::mat4 rotMtx = glm::toMat4(pNode->Orientation.Quat);
					glm::extractEulerAngleXYZ(rotMtx, pNode->Orientation.Angles.x, pNode->Orientation.Angles.y, pNode->Orientation.Angles.z);
					pNode->Orientation.Angles = glm::degrees(pNode->Orientation.Angles);
					pNode->Orientation.Mode = ORIENT_XYZ;
				}
			}

			for (uint m = 0; m < aNode->mNumMeshes; m++)
			{
				auto aMesh = pScene->mMeshes[aNode->mMeshes[m]];
				MeshRenderer* pRenderer = pNode->AddComponent(new MeshRenderer())->As<MeshRenderer>();

				auto foundMesh = _meshFixup.find(aMesh);
				if (foundMesh == _meshFixup.end())
				{
					Mesh* pMesh = resMgr.AddMesh(aMesh->mName.C_Str());
					ParseMesh(aMesh, pMesh);
					_meshFixup[aMesh] = pMesh;
					pRenderer->SetMesh(pMesh);
				}
				else
				{
					pRenderer->SetMesh((*foundMesh).second);
				}

				auto aMaterial = pScene->mMaterials[aMesh->mMaterialIndex];
				auto foundMaterial = _materialFixup.find(aMaterial);
				if (foundMaterial == _materialFixup.end())
				{
					Material* pMaterial = resMgr.AddMaterial(aMaterial->GetName().C_Str());
					StrMap<Pair<Texture2D*, bool>> textureLoadMap;
					ParseMaterial(aMaterial, pMaterial, textureLoadMap, fileDir);

					StrMap<Texture2D*> textureMap;
					for (auto& tex : textureLoadMap)
					{
						if (tex.second.second)
						{
							Texture2D* pTexture = tex.second.first;
							pTexture->SetUserDataPtr(this);
							tp.AddTask([](uint, void* pData) -> void {
								Texture2D* pTexture = static_cast<Texture2D*>(pData);
								AssetImporter* pThis = static_cast<AssetImporter*>(pTexture->GetUserDataPtr());
								if (pTexture->LoadFromFile())
								{
									uint maxSize = pThis->_options.MaxTextureSize;
									if (pTexture->GetWidth() > maxSize || pTexture->GetHeight() > maxSize)
									{
										pTexture->Resize(glm::min(pTexture->GetWidth(), maxSize), glm::min(pTexture->GetHeight(), maxSize));
									}
									pTexture->GenerateMips(false);

									if ((*pThis->_textureLoadList.find(pTexture)).second != MaterialStrings::NormalMap)
										pTexture->Compress();
								}
							}, pTexture);
							_textureLoadList[pTexture] = tex.first;
						}
						textureMap[tex.first] = tex.second.first;
					}
					_materialFixup[aMaterial] = { pMaterial, textureMap };
					pRenderer->SetMaterial(pMaterial);
				}
				else
				{
					pRenderer->SetMaterial((*foundMaterial).second.first);
				}
			}
		}

		tp.Wait();

		for (auto& pTexture : _textureLoadList)
		{
			if (!pTexture.first->RegisterToGPU())
				return false;
		}

		for (auto& mtl : _materialFixup)
		{
			auto& textures = mtl.second.second;
			for (auto& tex : textures)
			{
				mtl.second.first->SetTexture2D(tex.first, tex.second);
			}
		}

		for (auto& node : _nodeFixup)
		{
			auto aNode = static_cast<aiNode*>(node.first);
			if (aNode->mParent)
				_asset->SetParent(node.second->GetName(), _nodeFixup[aNode->mParent]->GetName());
		}

		return true;
	}

	bool AssetImporter::ChooseMaterial(void*, Material*&)
	{
		return false;
	}
}
#endif