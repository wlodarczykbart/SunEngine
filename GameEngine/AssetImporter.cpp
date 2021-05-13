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
			if (!pTexture->RegisterToGPU())
				return false;
		}

		for (auto& mtl : _materialCache)
		{
			auto& textures = mtl.second.second;
			for (auto& tex : textures)
				mtl.first->SetTexture2D(tex.first, tex.second);
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
		}

		if (pMesh->MeshData->VertexBones.size())
		{
			if (shader == DefaultShaders::Metallic) shader = DefaultShaders::SkinnedMetallic;
			else if (shader == DefaultShaders::Specular) shader = DefaultShaders::SkinnedSpecular;
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

				if ((*iter).first == MaterialStrings::DiffuseMap)
					pTex->SetSRGB(true);

				ThreadPool& tp = ThreadPool::Get();
				tp.AddTask([](uint, void* pData) -> void {
					Texture2D* pTexture = static_cast<Texture2D*>(pData);
					if (pTexture->LoadFromFile())
					{
						pTexture->GenerateMips(false);
					}
				}, pTex);

				_textureLoadList.push_back(pTex);
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