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

			VertexDef vtxDef = StandardVertex::Definition;
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
					pSkinned->SetMesh(pRenderer->GetMesh());
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

		for (auto& mtl : _materialFixup)
		{
			auto& textures = mtl.second.second;
			for (auto& tex : textures)
			{
				mtl.second.first->SetTexture2D(tex.first, tex.second);
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

			if (pMtl->SpecularFactorMap) importerTextures[MaterialStrings::MetallicMap] = pMtl->SpecularFactorMap;
			else if (pMtl->SpecularMap) importerTextures[MaterialStrings::MetallicMap] = pMtl->SpecularMap;

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

		for (auto& cached : _materialFixup)
		{
			bool equalShader = shader == cached.second.first->GetShader()->GetName();
			bool equalMaterial = _options.CombineMaterials ? (cached.first == pMtl || ImportedMaterialsSame(pMtl, (ModelImporter::Importer::Material*)cached.second.first)) : cached.first == pMtl;
			if (equalShader && equalMaterial)
			{
				pOutMtl = cached.second.first;
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
					pTex->SetSRGB();

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
		_materialFixup[pMtl] = { pOutMtl, textures };
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
	namespace CustomTextureTypes
	{
		const String Roughness = "Roughness";
		const String DiffuseRoughness = "DiffuseRoughness";
		const String RoughnessMetallic = "RoughnessMetallic";
		const String OcclusionRoughnessMetallic = "OcclusionRoughnessMetallic";
		const String SpecularGlossiness = "SpecularGlossiness";
		const String MetallicRoughness = "MetallicRoughness";
		const String Invalid = "Invalid";
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

	glm::vec3 FromAssimp(const aiVector3D& v)
	{
		return glm::vec3(v.x, v.y, v.z);
	}

	bool IsMaterialKey(const aiString& str, const char* pKeyName, int, int)
	{
		return strcmp(str.C_Str(), pKeyName) == 0;
	}

	void CollectNodes(aiNode* pNode, Vector<aiNode*>& nodes)
	{
		nodes.push_back(pNode);

		for (uint i = 0; i < pNode->mNumChildren; i++)
			CollectNodes(pNode->mChildren[i], nodes);
	}

	bool ParseMesh(aiMesh* pSrc, Mesh* pDst, Material* pMtl, const StrMap<uint>& boneIndexLookup)
	{
		bool skinSupport = boneIndexLookup.size() > 0;
		pDst->AllocVertices(pSrc->mNumVertices, pSrc->HasBones() && skinSupport ? SkinnedVertex::Definition : StandardVertex::Definition);

		for (uint i = 0; i < pSrc->mNumVertices; i++)
		{
			pDst->SetVertexVar(i, glm::vec4(FromAssimp(pSrc->mVertices[i]), 1.0f));
			if (pSrc->HasTextureCoords(0))
			{
				glm::vec4 uv = glm::vec4(FromAssimp(pSrc->mTextureCoords[0][i]), 0.0f);
				uv.y = 1.0f - uv.y;
				pDst->SetVertexVar(i, uv, VertexDef::DEFAULT_TEX_COORD_INDEX);
			}
			else
			{
				pDst->SetVertexVar(i, Vec4::Zero, VertexDef::DEFAULT_TEX_COORD_INDEX);
			}

			if(pSrc->HasNormals())
				pDst->SetVertexVar(i, glm::vec4(FromAssimp(pSrc->mNormals[i]), 0.0f), VertexDef::DEFAULT_NORMAL_INDEX);
			else
				pDst->SetVertexVar(i, Vec4::Up, VertexDef::DEFAULT_NORMAL_INDEX);

			if(pSrc->HasTangentsAndBitangents())
				pDst->SetVertexVar(i, glm::vec4(FromAssimp(pSrc->mTangents[i]), 0.0f), VertexDef::DEFAULT_TANGENT_INDEX);
			else
				pDst->SetVertexVar(i, Vec4::Right, VertexDef::DEFAULT_TANGENT_INDEX);
		}

		if (pSrc->HasBones() && skinSupport)
		{
			struct VertexBoneData
			{
				uint bones[4];
				float weights[4];
				int minWeightIndex;

				void Normalize()
				{
					float sum = 0.0f;
					for (uint i = 0; i < 4; i++)
						sum += weights[i];

					for (uint i = 0; i < 4; i++)
						weights[i] /= sum;
				}
			};

			Vector<VertexBoneData> boneDataList;
			boneDataList.resize(pSrc->mNumVertices);
			memset(boneDataList.data(), 0x0, sizeof(VertexBoneData) * boneDataList.size());

			for (uint i = 0; i < pSrc->mNumBones; i++)
			{
				auto& bone = pSrc->mBones[i];
				uint boneIndex = boneIndexLookup.at(bone->mName.C_Str());
				for (uint j = 0; j < bone->mNumWeights; j++)
				{
					auto& weight = bone->mWeights[j];
					auto& boneData = boneDataList[weight.mVertexId];
					if (weight.mWeight > boneData.weights[boneData.minWeightIndex])
					{
						boneData.weights[boneData.minWeightIndex] = weight.mWeight;
						boneData.bones[boneData.minWeightIndex] = boneIndex;

						float minWeight = FLT_MAX;
						for (uint k = 0; k < 4; k++)
						{
							if (boneData.weights[k] < minWeight)
							{
								minWeight = boneData.weights[k];
								boneData.minWeightIndex = k;
							}
						}
					}
				}
			}

			SkinnedVertex* pSkinnedVerts = pDst->GetVertices<SkinnedVertex>();
			for (uint i = 0; i < boneDataList.size(); i++)
			{
				auto& boneData = boneDataList[i];
				boneData.Normalize();
				pSkinnedVerts[i].Bones = glm::vec4(boneData.bones[0], boneData.bones[1], boneData.bones[2], boneData.bones[3]);
				pSkinnedVerts[i].Weights = glm::vec4(boneData.weights[0], boneData.weights[1], boneData.weights[2], boneData.weights[3]);
			}
		}

		Vector<uint> indexBuffer;

		switch (pSrc->mPrimitiveTypes)
		{
		case aiPrimitiveType_POINT:
			pDst->SetPrimitiveToplogy(SE_PT_POINT_LIST);
			indexBuffer.reserve(pSrc->mNumFaces);
			break;
		case aiPrimitiveType_LINE:
			pDst->SetPrimitiveToplogy(SE_PT_LINE_LIST);
			indexBuffer.reserve(pSrc->mNumFaces * 2);
			break;
		case aiPrimitiveType_TRIANGLE:
			pDst->SetPrimitiveToplogy(SE_PT_TRIANGLE_LIST);
			indexBuffer.reserve(pSrc->mNumFaces * 3);
			break;
		default:
			return false;
		}

		for (uint i = 0; i < pSrc->mNumFaces; i++)
		{
			auto& face = pSrc->mFaces[i];
			for (uint j = 0; j < face.mNumIndices; j++)
				indexBuffer.push_back(face.mIndices[j]);
		}

		pDst->AllocIndices(indexBuffer.size());
		pDst->SetIndices(indexBuffer.data(), 0, indexBuffer.size());

		pDst->UpdateBoundingVolume();

		//auto* alpha = pMtl->GetTexture2D(MaterialStrings::AlphaMap);
		//if(alpha && alpha->GetName() != DefaultResource::Texture::White)
		//	pDst->

		if (!pDst->RegisterToGPU())
			return false;

		return true;
	}

	void ClearPixelGBA(Pixel& p)
	{
		p.G = 0;
		p.B = 0;
		p.A = 255;
	}

	void ClearPixelA(Pixel& p)
	{
		p.A = 255;
	}

	bool AssetImporter::ChooseMaterial(void* pSrcPtr, Material*& pDst)
	{
		aiMaterial* pSrc = (aiMaterial*)pSrcPtr;

		unsigned int iUV;
		float fBlend;
		aiTextureOp eOp;
		aiString szPath;
		Map<aiTextureType, Vector<String>> textureMap;
		HashSet<String> foundTextures;
		for (unsigned int i = 0; i <= AI_TEXTURE_TYPE_MAX; ++i)
		{
			unsigned int iNum = 0;
			while (true)
			{
				if (AI_SUCCESS != aiGetMaterialTexture(pSrc, (aiTextureType)i, iNum,
					&szPath, nullptr, &iUV, &fBlend, &eOp))
				{
					break;
				}

				if (foundTextures.count(szPath.C_Str()) == 0)
				{
					textureMap[aiTextureType(i)].push_back(szPath.C_Str());
					foundTextures.insert(szPath.C_Str());
				}
				++iNum;
			}
		}

		static const Map<aiTextureType, String> CommonTextureTypes =
		{
			{ aiTextureType_DIFFUSE, MaterialStrings::DiffuseMap },
			{ aiTextureType_SPECULAR, MaterialStrings::SpecularMap },
			{ aiTextureType_NORMALS, MaterialStrings::NormalMap },
			{ aiTextureType_OPACITY, MaterialStrings::AlphaMap },
			{ aiTextureType_SHININESS, MaterialStrings::GlossMap }, //need some conversion from shininess to a value to be used in pbr? as of this comment specular shader doesnt take smoothness texture
			{ aiTextureType_BASE_COLOR, MaterialStrings::DiffuseMap },
			{ aiTextureType_METALNESS, MaterialStrings::MetallicMap },
			{ aiTextureType_AMBIENT_OCCLUSION, MaterialStrings::AmbientOcclusionMap },
			{ aiTextureType_LIGHTMAP, MaterialStrings::AmbientOcclusionMap },
			{ aiTextureType_EMISSION_COLOR, MaterialStrings::EmissiveMap },
			{ aiTextureType_EMISSIVE, MaterialStrings::EmissiveMap },
			{ aiTextureType_DIFFUSE_ROUGHNESS, CustomTextureTypes::DiffuseRoughness },
		};

		String fileDir = GetDirectory(_path) + "/";

		for (auto& texMapping : textureMap)
		{
			for (auto& path : texMapping.second)
			{
				String strPath;
				FileStream fs;
				if (fs.OpenForRead(path.c_str())) //easiest case if file path is correct
				{
					strPath = path;
					fs.Close();
				}
				else if (fs.OpenForRead((fileDir + path).c_str())) //second easiest case if the file is relative to the input file directory
				{
					strPath = fileDir + path;
					fs.Close();
				}
				else //try to find the file somewhere in the input file directory
				{
					String filename = StrToLower(GetFileName(path));
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
						needsLoad = true;
					}

					String texType;

					//namespace CustomTextureTypes
					//{
					//	const String Roughness = "Roughness";
					//	const String DiffuseRoughness = "DiffuseRoughness";
					//	const String RoughnessMetallic = "RoughnessMetallic";
					//	const String OcclusionRoughnessMetallic = "OcclusionRoughnessMetallic";
					//	const String SpecularGlossiness = "SpecularGlossiness";
					//	const String MetallicRoughness = "MetallicRoughness";
					//}

					//try to determine
					String texName = StrToLower(GetFileNameNoExt(strPath));

					usize posDiffuse = texName.find("diffuse");
					usize posSpec = texName.find("spec");
					usize posGloss = texName.find("gloss");
					usize posRough = texName.find("rough");
					usize posMetal = texName.find("metal");
					usize posAO = texName.find("ambient");
					if (posAO == String::npos) posAO = texName.find("occlusion");

					if (posAO != String::npos && posRough != String::npos && posMetal != String::npos)
					{
						texType = CustomTextureTypes::OcclusionRoughnessMetallic; //TODO: add other ordering?
					}
					else if (posRough != String::npos && posMetal != String::npos)
					{
						texType = posRough < posMetal ? CustomTextureTypes::RoughnessMetallic : CustomTextureTypes::MetallicRoughness;
					}
					else if (posDiffuse != String::npos && posRough != String::npos)
					{
						texType = CustomTextureTypes::DiffuseRoughness;
					}
					else if (posRough != String::npos)
					{
						texType = CustomTextureTypes::Roughness;
					}
					else if (posSpec != String::npos && posGloss != String::npos)
					{
						texType = CustomTextureTypes::SpecularGlossiness;
					}
					else if (CommonTextureTypes.count(texMapping.first))
					{
						texType = CommonTextureTypes.at(texMapping.first);
					}
					else
					{
						texType = CustomTextureTypes::Invalid;
					}

					Vector<TextureLoadTask> tasks;

					if (texType == CustomTextureTypes::OcclusionRoughnessMetallic)
					{
						Texture2D* pTextureAO = needsLoad ? resMgr.AddTexture2D(strPath + ".OCCLUSION") : resMgr.GetTexture2D(strPath + ".OCCLUSION");
						Texture2D* pTextureRough = needsLoad ? resMgr.AddTexture2D(strPath + ".ROUGHNESS") : resMgr.GetTexture2D(strPath + ".ROUGHNESS");
						Texture2D* pTextureMetal = needsLoad ? resMgr.AddTexture2D(strPath + ".METAL") : resMgr.GetTexture2D(strPath + ".METAL");
						if (needsLoad)
						{
							tasks.push_back(TextureLoadTask(pTextureAO, TC_RED, TC_RED, TC_RED, TC_RED, true, false, ClearPixelA));
							tasks.push_back(TextureLoadTask(pTextureRough, TC_GREEN, TC_GREEN, TC_GREEN, TC_GREEN, true, false, ClearPixelA));
							tasks.push_back(TextureLoadTask(pTextureMetal, TC_BLUE, TC_BLUE, TC_BLUE, TC_BLUE, true, false, ClearPixelA));
						}
						_materialMapping[pDst].push_back({ MaterialStrings::AmbientOcclusionMap, pTextureAO });
						_materialMapping[pDst].push_back({ MaterialStrings::RoughnessMap, pTextureRough });
						_materialMapping[pDst].push_back({ MaterialStrings::MetallicMap, pTextureMetal });
					}
					else if (texType == CustomTextureTypes::RoughnessMetallic)
					{
						Texture2D* pTextureRough = needsLoad ? resMgr.AddTexture2D(strPath + ".ROUGHNESS") : resMgr.GetTexture2D(strPath + ".ROUGHNESS");
						Texture2D* pTextureMetal = needsLoad ? resMgr.AddTexture2D(strPath + ".METAL") : resMgr.GetTexture2D(strPath + ".METAL");
						if (needsLoad)
						{
							tasks.push_back(TextureLoadTask(pTextureRough, TC_RED, TC_RED, TC_RED, TC_RED, true, false, ClearPixelA));
							tasks.push_back(TextureLoadTask(pTextureMetal, TC_GREEN, TC_GREEN, TC_GREEN, TC_GREEN, true, false, ClearPixelA));
						}
						_materialMapping[pDst].push_back({ MaterialStrings::RoughnessMap, pTextureRough });
						_materialMapping[pDst].push_back({ MaterialStrings::MetallicMap, pTextureMetal });
					}
					else if (texType == CustomTextureTypes::MetallicRoughness)
					{
						Texture2D* pTextureMetal = needsLoad ? resMgr.AddTexture2D(strPath + ".METAL") : resMgr.GetTexture2D(strPath + ".METAL");
						Texture2D* pTextureRough = needsLoad ? resMgr.AddTexture2D(strPath + ".ROUGHNESS") : resMgr.GetTexture2D(strPath + ".ROUGHNESS");
						if (needsLoad)
						{
							tasks.push_back(TextureLoadTask(pTextureMetal, TC_RED, TC_RED, TC_RED, TC_RED, true, false, ClearPixelA));
							tasks.push_back(TextureLoadTask(pTextureRough, TC_GREEN, TC_GREEN, TC_GREEN, TC_GREEN, true, false, ClearPixelA));
						}
						_materialMapping[pDst].push_back({ MaterialStrings::MetallicMap, pTextureMetal });
						_materialMapping[pDst].push_back({ MaterialStrings::RoughnessMap, pTextureRough });
					}
					else if (texType == CustomTextureTypes::DiffuseRoughness)
					{
						Texture2D* pTextureDiffue = needsLoad ? resMgr.AddTexture2D(strPath + ".DIFFUSE") : resMgr.GetTexture2D(strPath + ".DIFFUSE");
						Texture2D* pTextureRough = needsLoad ? resMgr.AddTexture2D(strPath + ".ROUGHNESS") : resMgr.GetTexture2D(strPath + ".ROUGHNESS");
						if (needsLoad)
						{
							tasks.push_back(TextureLoadTask(pTextureDiffue, TC_RED, TC_GREEN, TC_BLUE, TC_ALPHA, true, true, ClearPixelA));
							tasks.push_back(TextureLoadTask(pTextureRough, TC_ALPHA, TC_ALPHA, TC_ALPHA, TC_ALPHA, true, false, ClearPixelA));
						}
						_materialMapping[pDst].push_back({ MaterialStrings::DiffuseMap, pTextureDiffue });
						_materialMapping[pDst].push_back({ MaterialStrings::RoughnessMap, pTextureRough });
					}
					else if (texType == CustomTextureTypes::Roughness)
					{
						Texture2D* pTextureRough = pTexture;
						if (needsLoad)
						{
							tasks.push_back(TextureLoadTask(pTextureRough, TC_RED, TC_GREEN, TC_BLUE, TC_ALPHA, true, false));
						}
						_materialMapping[pDst].push_back({ MaterialStrings::RoughnessMap, pTextureRough });
					}
					else if (texType == CustomTextureTypes::SpecularGlossiness)
					{
						Texture2D* pTextureSpecular = needsLoad ? resMgr.AddTexture2D(strPath + ".SPECULAR") : resMgr.GetTexture2D(strPath + ".SPECULAR");
						Texture2D* pTextureGloss = needsLoad ? resMgr.AddTexture2D(strPath + ".GLOSS") : resMgr.GetTexture2D(strPath + ".GLOSS");
						if (needsLoad)
						{
							tasks.push_back(TextureLoadTask(pTextureSpecular, TC_RED, TC_RED, TC_RED, TC_RED, true, false, ClearPixelA));
							tasks.push_back(TextureLoadTask(pTextureGloss, TC_GREEN, TC_GREEN, TC_GREEN, TC_GREEN, true, false, ClearPixelA));
						}
						_materialMapping[pDst].push_back({ MaterialStrings::SpecularMap, pTextureSpecular });
						_materialMapping[pDst].push_back({ MaterialStrings::GlossMap, pTextureGloss });
					}
					else if(texType != CustomTextureTypes::Invalid)
					{
						if (needsLoad)
						{
							tasks.push_back(TextureLoadTask(pTexture, TC_RED, TC_GREEN, TC_BLUE, TC_ALPHA, texType != MaterialStrings::NormalMap, texType == MaterialStrings::DiffuseMap));
						}
						_materialMapping[pDst].push_back({ texType, pTexture });
					}

					if (needsLoad)
					{
						_textureLoadList[pTexture] = strPath;
						_textureLoadTasks[pTexture] = tasks;
					}
				}
			}
		}

		//TODO: better way to determine if metal shader?
		bool metallic = false;
		bool alphaTest = false;
		bool emissive = false;
		for (auto& texType : _materialMapping[pDst])
		{
			if (texType.first == MaterialStrings::MetallicMap || texType.first == MaterialStrings::RoughnessMap)
				metallic = true;
			if (texType.first == MaterialStrings::AlphaMap)
				alphaTest = true;
			if (texType.first == MaterialStrings::EmissiveMap)
				emissive = true;
		}

		String strShader = metallic ? DefaultShaders::Metallic : DefaultShaders::Specular;
		pDst->SetShader(ShaderMgr::Get().GetShader(strShader));
		if (!pDst->RegisterToGPU())
			return false;

		pDst->GetShader()->SetDefaults(pDst);

		for (uint i = 0; i < pSrc->mNumProperties; i++)
		{
			auto prop = pSrc->mProperties[i];
			//printf("%s\t", prop->mKey.C_Str());		

			switch (prop->mType)
			{
				//only supporting int or float types...
			case aiPTI_Float:
			case aiPTI_Integer:
				break;
			default:
				continue;
			}

			//if(IsMaterialKey(prop->mKey, AI_MATKEY_COLOR_DIFFUSE))
			//	pDst->SetMaterialVar(MaterialStrings::DiffuseColor, prop->mData, prop->mDataLength);

			//else if (IsMaterialKey(prop->mKey, AI_MATKEY_COLOR_SPECULAR))
			//	pDst->SetMaterialVar(MaterialStrings::SpecularColor, prop->mData, prop->mDataLength);

			//else if (IsMaterialKey(prop->mKey, AI_MATKEY_OPACITY))
			//	pDst->SetMaterialVar(MaterialStrings::Opaqueness, prop->mData, prop->mDataLength);

			//else if (IsMaterialKey(prop->mKey, AI_MATKEY_COLOR_TRANSPARENT))
			//	pDst->SetMaterialVar(MaterialStrings::Opaqueness, prop->mData, prop->mDataLength);

			//else if (IsMaterialKey(prop->mKey, AI_MATKEY_SHININESS))
			//	pDst->SetMaterialVar(MaterialStrings::Smoothness, prop->mData, prop->mDataLength);

			//else if (IsMaterialKey(prop->mKey, AI_MATKEY_SHININESS_STRENGTH))
			//	pDst->SetMaterialVar(MaterialStrings::Smoothness, prop->mData, prop->mDataLength);
		}

		////handle phong specualar exponent this way for now...
		//if (smoothness.r > 1.0f)
		//	smoothness.r = 1.0f - expf(-smoothness.r * 0.008f);

		//pDst->SetMaterialVar(MaterialStrings::DiffuseColor, Vec4::Zero);
		//pDst->SetMaterialVar(MaterialStrings::SpecularColor, specularColor);
		//pDst->SetMaterialVar(MaterialStrings::Smoothness, smoothness);

		if (alphaTest)
		{
			float opacity;
			pDst->GetMaterialVar(MaterialStrings::Opacity, opacity);
			opacity *= 2.0f;
			pDst->SetMaterialVar(MaterialStrings::Opacity, opacity);
		}

		return true;
	}

	bool AssetImporter::Import(const String& filename, const Options& options)
	{
		_options = options;

		auto pScene = aiImportFile(filename.c_str(), aiProcessPreset_TargetRealtime_MaxQuality/* | aiProcess_ConvertToLeftHanded*/);
		if (!pScene)
			return false;

		//auto pScene = (aiScene*)aiImportFile(filename.c_str(),
		//	37690883 | /* configurable pp steps */
		//	aiProcess_GenSmoothNormals | // generate smooth normal vectors if not existing
		//	aiProcess_SplitLargeMeshes | // split large, unrenderable meshes into submeshes
		//	aiProcess_Triangulate | // triangulate polygons with more than 3 edges
		//	//aiProcess_ConvertToLeftHanded | // convert everything to D3D left handed space
		//	aiProcess_SortByPType | // make 'clean' meshes which consist of a single typ of primitives
		//	0);

		bool lights = pScene->HasLights();
		bool cameras = pScene->HasCameras();

		auto& resMgr = ResourceMgr::Get();

		_path = filename;
		_asset = resMgr.AddAsset(GetFileNameNoExt(filename));

		Vector<aiNode*> nodes;
		CollectNodes(pScene->mRootNode, nodes);

		String fileDir = GetDirectory(filename) + "/";

		Animator* pAnimator = 0;
		StrMap<Map<uint, aiNodeAnim*>> boneMapping;
		StrMap<uint> boneIndexLookup;
		Map<aiMesh*, uint> skinnedMeshLookup;

		//NOTE: have seen several fbx models that assimp does not produce correct animation data for when comparing my assimp code
		//with the assimp project test viewer and getting same incorrect results compared to correct results from my 3DImporter class.
		if (pScene->HasAnimations())
		{
			pAnimator = new Animator();
			Vector<AnimationClip> clips;

			for (uint i = 0; i < pScene->mNumAnimations; i++)
			{
				auto aAnim = pScene->mAnimations[i];

				uint keyCount = 0;
				double startTime = FLT_MAX;
				double endTime = -FLT_MAX;

				for (uint j = 0; j < aAnim->mNumChannels; j++)
				{
					auto aChannel = aAnim->mChannels[j];
					keyCount = glm::max(glm::max(glm::max(keyCount, aChannel->mNumPositionKeys), aChannel->mNumRotationKeys), aChannel->mNumScalingKeys);

					startTime = glm::min(glm::min(glm::min(startTime,
						aChannel->mPositionKeys[0].mTime),
						aChannel->mRotationKeys[0].mTime),
						aChannel->mScalingKeys[0].mTime);

					endTime = glm::max(glm::max(glm::max(endTime,
						aChannel->mPositionKeys[aChannel->mNumPositionKeys - 1].mTime),
						aChannel->mRotationKeys[aChannel->mNumRotationKeys - 1].mTime),
						aChannel->mScalingKeys[aChannel->mNumScalingKeys - 1].mTime);

					boneMapping[aChannel->mNodeName.C_Str()][i] = aChannel;
				}

				if (keyCount > 1)
				{
					AnimationClip clip;
					clip.SetName(aAnim->mName.C_Str());

					Vector<float> keys;
					keys.resize(keyCount);
					float keyFactor = (float)1.0f / (keyCount - 1);;
					for (uint j = 0; j < keyCount; j++)
					{
						keys[j] = (float)glm::mix(startTime, endTime, double(j * keyFactor));
						keys[j] /= (float)aAnim->mTicksPerSecond;
					}

					clip.SetKeys(keys);
					clips.push_back(clip);
				}
			}

			for (uint i = 0; i < pScene->mNumMeshes; i++)
			{
				if (pScene->mMeshes[i]->HasBones())
				{
					skinnedMeshLookup[pScene->mMeshes[i]] = skinnedMeshLookup.size();
					for (uint j = 0; j < pScene->mMeshes[i]->mNumBones; j++)
					{
						if (boneIndexLookup.find(pScene->mMeshes[i]->mBones[j]->mName.C_Str()) == boneIndexLookup.end())
							boneIndexLookup[pScene->mMeshes[i]->mBones[j]->mName.C_Str()] = boneIndexLookup.size();
					}
				}
			}

			pAnimator->SetClips(clips);
			pAnimator->SetBoneCount(boneIndexLookup.size());
		}

		for (auto aNode : nodes)
		{
			String strName = aNode->mName.length ? aNode->mName.C_Str() : StrFormat("%p", aNode);
			auto pNode = _asset->AddNode(strName);
			_nodeFixup[aNode] = pNode;

			glm::mat4 mtxLocal;
			memcpy(&mtxLocal, &aNode->mTransformation, sizeof(glm::mat4));
			mtxLocal = glm::transpose(mtxLocal);

			pNode->Position = mtxLocal[3];
			pNode->Scale.x = glm::length(mtxLocal[0]);
			pNode->Scale.y = glm::length(mtxLocal[1]);
			pNode->Scale.z = glm::length(mtxLocal[2]);

			mtxLocal[0]	/= pNode->Scale.x;
			mtxLocal[1]	/= pNode->Scale.y;
			mtxLocal[2]	/= pNode->Scale.z;
			mtxLocal[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

			pNode->Orientation.Quat = glm::quat_cast(mtxLocal);
			pNode->Orientation.Mode = ORIENT_QUAT;

			//glm::mat4 rebuilt = glm::transpose(pNode->BuildLocalMatrix());
			//bool equal =  reinterpret_cast<aiMatrix4x4*>(&rebuilt)->Equal(aNode->mTransformation);
			//assert(equal);

			if (aNode == pScene->mRootNode)
			{
				glm::extractEulerAngleXYZ(mtxLocal, pNode->Orientation.Angles.x, pNode->Orientation.Angles.y, pNode->Orientation.Angles.z);
				pNode->Orientation.Angles = glm::degrees(pNode->Orientation.Angles);
				pNode->Orientation.Mode = ORIENT_XYZ;
			}

			for (uint m = 0; m < aNode->mNumMeshes; m++)
			{
				auto aMesh = pScene->mMeshes[aNode->mMeshes[m]];
				MeshRenderer* pRenderer = pNode->AddComponent(new MeshRenderer())->As<MeshRenderer>();

				auto aMaterial = pScene->mMaterials[aMesh->mMaterialIndex];
				auto foundMaterial = _materialFixup.find(aMaterial);
				if (foundMaterial == _materialFixup.end())
				{
					Material* pMaterial = resMgr.AddMaterial(aMaterial->GetName().C_Str());
					ChooseMaterial(aMaterial, pMaterial);
					_materialFixup[aMaterial].first = pMaterial;
					pRenderer->SetMaterial(pMaterial);
				}
				else
				{
					pRenderer->SetMaterial((*foundMaterial).second.first);
				}

				auto foundMesh = _meshFixup.find(aMesh);
				if (foundMesh == _meshFixup.end())
				{
					Material* pMaterial = pRenderer->GetMaterial();
					Mesh* pMesh = resMgr.AddMesh(aMesh->mName.C_Str());
					ParseMesh(aMesh, pMesh, pMaterial, boneIndexLookup);
					_meshFixup[aMesh] = pMesh;
					pRenderer->SetMesh(pMesh);
				}
				else
				{
					pRenderer->SetMesh((*foundMesh).second);
				}
				
				auto foundSkinned = skinnedMeshLookup.find(aMesh);
				if (foundSkinned != skinnedMeshLookup.end())
				{
					SkinnedMesh* pSkinnedMesh = pNode->AddComponent(new SkinnedMesh())->As<SkinnedMesh>();
					pSkinnedMesh->SetMesh(_meshFixup[aMesh]);
					pSkinnedMesh->SetSkinIndex((*foundSkinned).second);
				}

				//break;
			}

			auto foundBone = boneIndexLookup.find(strName);
			if (pAnimator && foundBone != boneIndexLookup.end())
			{
				AnimatedBone* pBone = pNode->AddComponent(new AnimatedBone())->As<AnimatedBone>();
				pBone->SetBoneIndex((*foundBone).second);

				Vector<Vector<AnimatedBone::Transform>> boneTransforms;
				boneTransforms.resize(pAnimator->GetClipCount());
				for (uint anim = 0; anim < boneTransforms.size(); anim++)
				{
					Vector<float> keys;
					pAnimator->GetClipKeys(anim, keys);
					boneTransforms[anim].resize(keys.size());

					auto foundMapping = boneMapping.find(strName);
					if (foundMapping != boneMapping.end())
					{
						auto& channels = (*foundMapping).second;
						if (channels.count(anim))
						{
							auto channel = channels[anim];
							for (uint key = 0; key < keys.size(); key++)
							{
								double time = (double)keys[key] * pScene->mAnimations[anim]->mTicksPerSecond; //AnimClip will have timings moved to seconds, undo for this since comparing assimp times
								auto& transform = boneTransforms[anim][key];

								bool foundPos = false;
								for (uint i = 0; i < channel->mNumPositionKeys - 1; i++)
								{
									auto& key0 = channel->mPositionKeys[i + 0];
									auto& key1 = channel->mPositionKeys[i + 1];
									if (time >= key0.mTime && time <= key1.mTime)
									{
										double t = (time - key0.mTime) / (key1.mTime - key0.mTime);
										glm::vec3 v0 = glm::vec3(key0.mValue.x, key0.mValue.y, key0.mValue.z);
										glm::vec3 v1 = glm::vec3(key1.mValue.x, key1.mValue.y, key1.mValue.z);
										transform.position = glm::mix(v0, v1, (float)t);
										foundPos = true;
										break;
									}
								}

								if (!foundPos)
									transform.position = glm::vec3(channel->mPositionKeys[0].mValue.x, channel->mPositionKeys[0].mValue.y, channel->mPositionKeys[0].mValue.z);

								bool foundScale = false;
								for (uint i = 0; i < channel->mNumScalingKeys - 1; i++)
								{
									auto& key0 = channel->mScalingKeys[i + 0];
									auto& key1 = channel->mScalingKeys[i + 1];
									if (time >= key0.mTime && time <= key1.mTime)
									{
										double t = (time - key0.mTime) / (key1.mTime - key0.mTime);
										glm::vec3 v0 = glm::vec3(key0.mValue.x, key0.mValue.y, key0.mValue.z);
										glm::vec3 v1 = glm::vec3(key1.mValue.x, key1.mValue.y, key1.mValue.z);
										transform.scale = glm::mix(v0, v1, (float)t);
										foundScale = true;
										break;
									}
								}

								if (!foundScale)
									transform.scale = glm::vec3(channel->mScalingKeys[0].mValue.x, channel->mScalingKeys[0].mValue.y, channel->mScalingKeys[0].mValue.z);

								bool foundRot = false;
								for (uint i = 0; i < channel->mNumRotationKeys - 1; i++)
								{
									auto& key0 = channel->mRotationKeys[i + 0];
									auto& key1 = channel->mRotationKeys[i + 1];
									if (time >= key0.mTime && time <= key1.mTime)
									{
										double t = (time - key0.mTime) / (key1.mTime - key0.mTime);
										glm::quat v0 = glm::quat(key0.mValue.w, key0.mValue.x, key0.mValue.y, key0.mValue.z);
										glm::quat v1 = glm::quat(key1.mValue.w, key1.mValue.x, key1.mValue.y, key1.mValue.z);
										transform.rotation = glm::slerp(v0, v1, (float)t);
										foundRot = true;
										break;
									}
								}

								if (!foundRot)
									transform.rotation = glm::quat(channel->mRotationKeys[0].mValue.w, channel->mRotationKeys[0].mValue.x, channel->mRotationKeys[0].mValue.y, channel->mRotationKeys[0].mValue.z);
							}
						}
					}
					else
					{
						//fill all keys with currente node transform
						for (uint key = 0; key < keys.size(); key++)
						{
							auto& transform = boneTransforms[anim][key];
							transform.position = pNode->Position;
							transform.scale = pNode->Scale;
							assert(pNode->Orientation.Mode == ORIENT_QUAT);
							transform.rotation = pNode->Orientation.Quat;
						}
					}
				}
				pBone->SetTransforms(boneTransforms);

				Vector<glm::mat4> skinMatrices;
				skinMatrices.resize(skinnedMeshLookup.size());
				for (uint i = 0; i < skinMatrices.size(); i++)
					skinMatrices[i] = Mat4::Identity;

				for (auto& sm : skinnedMeshLookup)
				{
					for (uint i = 0; i < sm.first->mNumBones; i++)
					{
						if (strName == sm.first->mBones[i]->mName.C_Str())
						{
							memcpy(&skinMatrices[sm.second], &sm.first->mBones[i]->mOffsetMatrix, sizeof(glm::mat4));
							skinMatrices[sm.second] = glm::transpose(skinMatrices[sm.second]);
							break;
						}
					}
				}
				pBone->SetSkinMatrices(skinMatrices);
			}
		}

		ThreadPool& tp = ThreadPool::Get();
		{
			for (auto& texData : _textureLoadList)
			{
				Texture2D* pTexture = texData.first;
				pTexture->SetUserDataPtr(this);
				tp.AddTask([](uint, void* pData) -> void 
				{
					Texture2D* pTexture = static_cast<Texture2D*>(pData);
					AssetImporter* pThis = static_cast<AssetImporter*>(pTexture->GetUserDataPtr());
					if (pTexture->LoadFromFile())
					{
						uint maxSize = pThis->_options.MaxTextureSize;
						if (pTexture->GetWidth() > maxSize || pTexture->GetHeight() > maxSize)
						{
							pTexture->Resize(glm::min(pTexture->GetWidth(), maxSize), glm::min(pTexture->GetHeight(), maxSize));
						}

						auto& tasks = pThis->_textureLoadTasks.at(pTexture);

						for (uint i = 0; i < tasks.size(); i++)
						{
							auto& task = tasks[i];
							if (task.Texture != pTexture)
							{
								Texture2D* pSubTexture = task.Texture;
								pSubTexture->Alloc(pTexture->GetWidth(), pTexture->GetHeight());

								for (uint y = 0; y < pTexture->GetHeight(); y++)
								{
									for (uint x = 0; x < pTexture->GetWidth(); x++)
									{
										Pixel srcPixel;
										pTexture->GetPixel(x, y, srcPixel);
										uchar* pSrcPixel = &srcPixel.R;
										Pixel dstPixel;
										dstPixel.R = pSrcPixel[task.R];
										dstPixel.G = pSrcPixel[task.G];
										dstPixel.B = pSrcPixel[task.B];
										dstPixel.A = pSrcPixel[task.A];
										if (task.TransformFunc) 
											task.TransformFunc(dstPixel);

										pSubTexture->SetPixel(x, y, dstPixel);
									}
								}

								pSubTexture->GenerateMips(false);
							}
							else
							{
								pTexture->GenerateMips(false);
							}

							if (task.Compress)
								task.Texture->Compress();

							if (task.SRGB)
								task.Texture->SetSRGB();
						}
					}
				}, pTexture);
			}
		}
		tp.Wait();

		HashSet<Texture2D*> registeredTextures;
		for (auto& mtlMap : _materialMapping)
		{
			auto pMaterial = mtlMap.first;
			auto& textures = mtlMap.second;
			for (auto& tex : textures)
			{
				if (registeredTextures.count(tex.second) == 0)
				{
					if (!tex.second->RegisterToGPU())
					{
						aiReleaseImport(pScene);
						return false;
					}
					registeredTextures.insert(tex.second);
				}
				pMaterial->SetTexture2D(tex.first, tex.second);
			}
		}

		for (auto& node : _nodeFixup)
		{
			auto aNode = static_cast<aiNode*>(node.first);
			if (aNode->mParent)
				_asset->SetParent(node.second->GetName(), _nodeFixup[aNode->mParent]->GetName());
		}

		if (pAnimator)
			_asset->GetRoot()->AddComponent(pAnimator);

		aiReleaseImport(pScene);

		return true;
	}
}
#endif