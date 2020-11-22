#include <assert.h>
#include <fstream>

#include "OBJImporter.h"
#include "FBXImporter.h"

#include "3DImporter.h"


const uint32_t MODEL_FILE_KEY = 0xDF4B8A;

namespace ModelImporter
{
	class Vec3Hasher
	{
	public:
		uint32_t operator()(const Vec3& v) const
		{
			uint32_t uiX = *((uint32_t*)&v.x);
			uint32_t uiY = *((uint32_t*)&v.y);
			uint32_t uiZ = *((uint32_t*)&v.z);

			return uiX + uiY + uiZ;
		}
	};

	class Vec2Hasher
	{
	public:
		uint32_t operator()(const Vec2& v) const
		{
			uint32_t uiX = *((uint32_t*)&v.x);
			uint32_t uiY = *((uint32_t*)&v.y);

			return uiX + uiY;
		}
	};

	Importer::Options::Options()
	{
		makeTwoSided = false;
	}

	Importer* Importer::Create(const std::string& filename)
	{

		std::string ext = StrUtil::ToLower(StrUtil::GetExtension(filename));

		Importer* pConverter = 0;

		if (ext == "obj")
			pConverter = new OBJImporter();
		else if (ext == "fbx")
			pConverter = new FBXImporter();

		if (pConverter)
			pConverter->_fileName = filename;

		return pConverter;

	}

	Importer::Options Importer::Options::Default()
	{
		Options defaultOpt;

		return defaultOpt;
	}

	bool Importer::Import(const Importer::Options& options)
	{
		_options = options;

		if (!this->DerivedImport())
			return false;

		//TODO add stuff that is for all converters...
		struct SharedPolygonVertex
		{
			PolygonVertex _vertex;
			uint32_t arrayIndex;
		};

		char stringBuffer[512];

		std::unordered_map<MeshInternalData*, MeshData*> meshDataMap;

		for(uint32_t m = 0; m < Meshes.size(); m++)
		{
			uint32_t numUniqueVertices = 0;
			Mesh* pMesh = Meshes[m];
			MeshInternalData* pMeshData = static_cast<MeshInternalData*>(pMesh->pUserData);

			std::unordered_map<MeshInternalData*, MeshData*>::iterator existingData = meshDataMap.find(pMeshData);
			if (existingData != meshDataMap.end())
			{
				pMesh->MeshData = (*existingData).second;
				continue;
			}

			this->RemoveDuplicateIndices(pMeshData);

			if (pMeshData->_normals.size() == 0)
			{
				this->ComputeNormals(pMeshData);
			}

			if (pMeshData->_tangents.size() == 0)
			{
				this->ComputeTangents(pMeshData);
			}

			MeshData* pOutputData = new MeshData();

			pOutputData->Indices.resize(pMeshData->_polyVerts.size());

			//TODO: move to own function?
			std::unordered_map<uint32_t, std::vector<SharedPolygonVertex> > polygonRemap;
			for (uint32_t v = 0; v < pMeshData->_polyVerts.size(); v++)
			{
				PolygonVertex& polyVert = pMeshData->_polyVerts[v];
				std::vector<SharedPolygonVertex>& sharedVerts = polygonRemap[polyVert.positionIndex];
				int foundIndex = -1;
				for (int j = 0; j < (int)sharedVerts.size() && foundIndex == -1; j++)
				{
					if (/* (sharedVerts[j]._vertex.positionIndex == polyVert.positionIndex) || */
						( sharedVerts[j]._vertex.normalIndex == polyVert.normalIndex && sharedVerts[j]._vertex.texCoordIndex == polyVert.texCoordIndex) )
						foundIndex = (int)sharedVerts[j].arrayIndex;
				}

				if (foundIndex == -1)
				{
					SharedPolygonVertex sharedVert;
					sharedVert.arrayIndex = numUniqueVertices++;
					sharedVert._vertex = polyVert;
					sharedVerts.push_back(sharedVert);
					foundIndex = (int)sharedVert.arrayIndex;
				}

				pOutputData->Indices[v] = foundIndex;
			}

			pOutputData->Vertices.resize(numUniqueVertices);
			if (pMeshData->_vertexBones.size())
			{
				pOutputData->VertexBones.resize(numUniqueVertices);
			}

			std::unordered_map<uint32_t, std::vector<SharedPolygonVertex>>::iterator remapIter = polygonRemap.begin();
			while (remapIter != polygonRemap.end())
			{
				std::vector<SharedPolygonVertex>& uniqueVerts = (*remapIter).second;
				for (uint32_t i = 0; i < uniqueVerts.size(); i++)
				{
					PolygonVertex& vertIndices = uniqueVerts[i]._vertex;
					uint32_t vertIndex = uniqueVerts[i].arrayIndex;

					pOutputData->Vertices[vertIndex].position = pMeshData->_positions[vertIndices.positionIndex];

					if (pOutputData->VertexBones.size())
					{
						pOutputData->VertexBones[vertIndex] = pMeshData->_vertexBones[vertIndices.positionIndex];
					}

					pOutputData->Vertices[vertIndex].normal = pMeshData->_normals[vertIndices.normalIndex];

					if (vertIndices.texCoordIndex != -1)
						pOutputData->Vertices[vertIndex].texCoord = pMeshData->_texCoords[vertIndices.texCoordIndex];

					if (pMeshData->_tangents.size())
					{
						Vec3 tangent = pMeshData->_tangents[vertIndices.positionIndex];
						tangent.Orthogonalize(pMeshData->_normals[vertIndices.normalIndex]);
						tangent.Norm();
						pOutputData->Vertices[vertIndex].tangent = tangent;
					}

				}
				remapIter++;
			}

			pOutputData->BoundingBox.Reset();
			for (uint32_t i = 0; i < pOutputData->Vertices.size(); i++)
			{
				pOutputData->BoundingBox.Update(pOutputData->Vertices[i].position);
			}

			if (options.makeTwoSided)
			{
				this->MakeTwoSided(pOutputData);
			}

			sprintf_s(stringBuffer, "MESH-%d", (uint32_t)meshDataMap.size());

			pOutputData->Name = stringBuffer;
			pOutputData->SkinIndex = pMeshData->_skinIndex;
			pMesh->MeshData = pOutputData;
			meshDataMap[pMeshData] = pOutputData;
			MeshDatas.push_back(pOutputData);
		}

		//make sure names are unique
		MakeNamesUnqiue(Nodes);
		MakeNamesUnqiue(Materials);

		//make sure we are only collecting materials in use by meshes
		std::vector<Material*> usedMaterials;
		for (uint32_t i = 0; i < Materials.size(); i++)
		{
			bool bUsed = false;
			for (uint32_t j = 0; j < Meshes.size() && !bUsed; j++)
			{
				if (Meshes[j]->Material == Materials[i])
					bUsed = true;
			}

			if (bUsed)
			{
				usedMaterials.push_back(Materials[i]);
			}
			else
			{
				delete Materials[i];
			}
		}
		Materials = usedMaterials;

		for (uint32_t i = 0; i < Materials.size(); i++)
		{
			Material* pMtl = Materials[i];

			//0 is not a valid spec exponent due to math issues, if this is set 
			//then make specular color black and exponent one to disable specular effect.
			if (pMtl->SpecularExponent == 0.0f)
			{
				pMtl->SpecularExponent = 1.0f;
				pMtl->SpecularColor.SetZero();
			}

			//clamp alpha
			pMtl->Alpha = fmaxf(fminf(pMtl->Alpha, 1.0f), 0.0f);
		}

		std::vector<Animation*> usedAnimations;
		for (uint32_t i = 0; i < Animations.size(); i++)
		{
			if (Animations[i]->KeyFrames.size())
			{
				usedAnimations.push_back(Animations[i]);
			}
			else
			{
				delete Animations[i];
			}
		}
		Animations = usedAnimations;

		std::vector<uint32_t> usedBones;
		usedBones.resize(Bones.size());
		memset(usedBones.data(), 0, sizeof(bool) * usedBones.size());

		for (uint32_t i = 0; i < MeshDatas.size(); i++)
		{
			MeshData* pMesh = MeshDatas[i];

			//if (pMesh->Parent && pMesh->Parent->GetType() == BONE)
			//{
			//	pMesh->LocalMatrix = pMesh->WorldMatrix;
			//	pMesh->Detach();
			//}

			for (uint32_t j = 0; j < pMesh->VertexBones.size(); j++)
			{
				uint32_t b = 0;
				while (b < VertexBoneInfo::MAX_VERTEX_BONES && pMesh->VertexBones[j].weights[b] > 0.0f)
				{
					usedBones[(uint32_t)pMesh->VertexBones[j].bones[b]]++;
					b++;
				}
			}
		}

		std::list<Node*> removeList;

		for (uint32_t i = 0; i < Bones.size(); i++)
		{
			if (IsBonePathRemoveable(Bones[i], usedBones.data()))
			{
				removeList.push_back(Bones[i]);
			}
		}


		RemoveNodes(removeList);

		//normalize the mesh data to a length of one
		//if (options.bNormalizeMeshes)
		//{
		//	AABB totalVolume;
		//	for (uint32_t i = 0; i < Meshes.size(); i++)
		//	{
		//		totalVolume.Update(Meshes[i]->BoundingBox);
		//	}

		//	Vec3 center = totalVolume.GetCenter();

		//	float maxDist = -FLT_MAX;
		//	for (uint32_t i = 0; i < Meshes.size(); i++)
		//	{
		//		for (uint32_t j = 0; j < Meshes[i]->Vertices.size(); j++)
		//		{
		//			float dist = (center - Meshes[i]->Vertices[j].position).GetLengthSquared();
		//			if (dist > maxDist)
		//				maxDist = dist;
		//		}
		//	}

		//	maxDist = sqrtf(maxDist);
		//	maxDist /= 2.0f;
		//	if (maxDist != 0.0f)
		//		maxDist = 1.0f / maxDist;

		//	for (uint32_t i = 0; i < Meshes.size(); i++)
		//	{
		//		Meshes[i]->BoundingBox.Reset();
		//		for (uint32_t j = 0; j < Meshes[i]->Vertices.size(); j++)
		//		{
		//			 Meshes[i]->Vertices[j].position *= maxDist;
		//			 Meshes[i]->BoundingBox.Update(Meshes[i]->Vertices[j].position);
		//		}
		//	}
		//}

		return true;
	}

	const std::string & Importer::GetFileName() const
	{
		return _fileName;
	}

	Importer::Bone * Importer::FindBone(const void* pUserData) const
	{
		for (uint32_t i = 0; i < Bones.size(); i++)
		{
			if (Bones[i]->pUserData == pUserData)
				return Bones[i];
		}

		return 0;
	}

	Importer::Bone * Importer::FindBone(const std::string& name) const
	{
		for (uint32_t i = 0; i < Bones.size(); i++)
		{
			if (Bones[i]->Name == name)
				return Bones[i];
		}

		return 0;
	}

	std::string Importer::GetUniqueNodeName(const std::string& base) const
	{
		uint32_t counter = 0;
		std::string uniqueName = base;
		bool nameExists;
		do
		{
			nameExists = false;
			for (uint32_t i = 0; i < Nodes.size() && !nameExists; i++)
			{
				if (Nodes[i]->Name == uniqueName)
					nameExists = true;
			}

			if(nameExists)
				uniqueName = StrUtil::Format("%s_%d", base.data(), ++counter);
		} while (nameExists);

		return uniqueName;
	}

	const  Importer::Options Importer::GetOptions() const
	{
		return _options;
	}

	bool Importer::GetFullFilePath(const std::string & inPath, std::string & ouPath) const
	{
		std::string filename = StrUtil::GetFileName(inPath);	
		std::string dir = StrUtil::GetDirectory(_fileName);
		std::string fullPath = dir;
		if (fullPath.back() != '/' && fullPath.back() != '\\')
			fullPath += "\\";
		fullPath += filename;

		std::ifstream fstream;

		bool bFileExists = false;

		fstream.open(fullPath);
		//if the file is not is same directoy as obj, we only attempt to check if its in some directory below
		if (!fstream.is_open())
		{
			std::string localFilename = StrUtil::GetDirectory(inPath);
			std::string localDir;
			while (!bFileExists)
			{
				size_t slashPos = localFilename.find_last_of("\\/");

				std::string localSubdirectory = slashPos != std::string::npos ? localFilename.substr(slashPos) : localFilename;
				localDir = localSubdirectory + localDir;
				fullPath = dir;
				if (fullPath.back() != '/' && fullPath.back() != '\\')
					fullPath += "\\";
				fullPath += localDir;
				if (fullPath.back() != '/' && fullPath.back() != '\\')
					fullPath += "\\";
				fullPath += filename;

				fstream.open(fullPath);
				if (fstream.is_open())
				{
					bFileExists = true;
					fstream.close();
				}

				if (slashPos == std::string::npos)
					break;

				localFilename = localFilename.substr(0, slashPos);
			}
		}
		else
		{
			bFileExists = true;
			fstream.close();
		}

		if (bFileExists)
			ouPath = fullPath;

		return bFileExists;
	}

	Importer::Material * Importer::GetMaterial(const std::string & name)
	{
		for (uint32_t i = 0; i < Materials.size(); i++)
		{
			if (Materials[i]->Name == name)
			{
				return Materials[i];
			}
		}

		return 0;
	}

	Importer::Material * Importer::GetMaterial(const void * pUserData)
	{
		for (uint32_t i = 0; i < Materials.size(); i++)
		{
			if (Materials[i]->pUserData == pUserData)
			{
				return Materials[i];
			}
		}

		return 0;
	}

	Importer::Texture * Importer::GetTexture(const std::string & filename)
	{
		for (uint32_t i = 0; i < Textures.size(); i++)
		{
			if (Textures[i]->FileName == filename)
			{
				return Textures[i];
			}
		}

		Texture* pTexture = new Texture(Textures.size());
		pTexture->FileName = filename;
		Textures.push_back(pTexture);
		return pTexture;
	}

	Importer::Mesh* Importer::AddMesh()
	{
		Mesh* pObj = new Mesh(GetMeshCount());
		Meshes.push_back(pObj);
		Nodes.push_back(pObj);
		return pObj;
	}

	Importer::Material* Importer::AddMaterial()
	{
		Material* pObj = new Material(GetMaterialCount());
		Materials.push_back(pObj);
		return pObj;
	}

	Importer::Texture* Importer::AddTexture()
	{
		Texture* pObj = new Texture(GetTextureCount());
		Textures.push_back(pObj);
		return pObj;
	}

	Importer::Animation* Importer::AddAnimation()
	{
		Animation* pObj = new Animation(GetAnimationCount());
		Animations.push_back(pObj);
		return pObj;
	}

	Importer::Bone* Importer::AddBone()
	{
		Bone* pObj = new Bone(GetBoneCount());
		Bones.push_back(pObj);
		Nodes.push_back(pObj);
		return pObj;
	}

	Importer::Node* Importer::AddNode()
	{
		Node* pObj = new Node(TRANSFORM, GetNodeCount());
		Nodes.push_back(pObj);
		return pObj;
	}

	void Importer::Destroy()
	{
		for (auto iter = Meshes.begin(); iter != Meshes.end(); ++iter)
		{
			delete *iter;
		}

		for (auto iter = Materials.begin(); iter != Materials.end(); ++iter)
		{
			delete *iter;
		}

		for (auto iter = Textures.begin(); iter != Textures.end(); ++iter)
		{
			delete *iter;
		}

		for (auto iter = Animations.begin(); iter != Animations.end(); ++iter)
		{
			delete *iter;
		}

		for (auto iter = Bones.begin(); iter != Bones.end(); ++iter)
		{
			delete *iter;
		}

		Meshes.clear();
		Materials.clear();
		Textures.clear();
		Animations.clear();
		Bones.clear();

	}

	Importer::Importer()
	{
	}

	void Importer::RemoveDuplicateIndices(MeshInternalData * pMesh)
	{
		std::vector<uint32_t> positionIndices;
		std::vector<uint32_t> normalIndices;
		std::vector<uint32_t> texCoordIndices;

		positionIndices.resize(pMesh->_positions.size());
		normalIndices.resize(pMesh->_normals.size());
		texCoordIndices.resize(pMesh->_texCoords.size());

		std::unordered_map<Vec3, uint32_t, Vec3Hasher> positionRemap;
		std::unordered_map<Vec3, uint32_t, Vec3Hasher> normalRemap;
		std::unordered_map<Vec2, uint32_t, Vec2Hasher> texCoordRemap;

		for (uint32_t i = 0; i < pMesh->_positions.size(); i++)
		{
			std::unordered_map<Vec3, uint32_t>::iterator idxIter = positionRemap.find(pMesh->_positions[i]);
			if (idxIter != positionRemap.end())
			{
				positionIndices[i] = (*idxIter).second;
			}
			else
			{
				positionIndices[i] = i;
				positionRemap[pMesh->_positions[i]] = i;
			}
		}

		for (uint32_t i = 0; i < pMesh->_normals.size(); i++)
		{
			std::unordered_map<Vec3, uint32_t>::iterator idxIter = normalRemap.find(pMesh->_normals[i]);
			if (idxIter != normalRemap.end())
			{
				normalIndices[i] = (*idxIter).second;
			}
			else
			{
				normalIndices[i] = i;
				normalRemap[pMesh->_normals[i]] = i;
			}
		}

		for (uint32_t i = 0; i < pMesh->_texCoords.size(); i++)
		{
			std::unordered_map<Vec2, uint32_t>::iterator idxIter = texCoordRemap.find(pMesh->_texCoords[i]);
			if (idxIter != texCoordRemap.end())
			{
				texCoordIndices[i] = (*idxIter).second;
			}
			else
			{
				texCoordIndices[i] = i;
				texCoordRemap[pMesh->_texCoords[i]] = i;
			}
		}

		for (uint32_t i = 0; i < pMesh->_polyVerts.size(); i++)
		{
			PolygonVertex& vtx = pMesh->_polyVerts[i];
			vtx.positionIndex = (int)positionIndices[vtx.positionIndex];

			if (vtx.normalIndex != -1)
				vtx.normalIndex = (int)normalIndices[vtx.normalIndex];

			if (vtx.texCoordIndex != -1)
				vtx.texCoordIndex = (int)texCoordIndices[vtx.texCoordIndex];
		}

	}

	void Importer::ComputeNormals(MeshInternalData* pMesh)
	{
		pMesh->_normals.resize(pMesh->_positions.size());
		for (uint32_t i = 0; i < pMesh->_polyVerts.size(); i += 3)
		{
			PolygonVertex &v0 = pMesh->_polyVerts[i + 0];
			PolygonVertex &v1 = pMesh->_polyVerts[i + 1];
			PolygonVertex &v2 = pMesh->_polyVerts[i + 2];

			Vec3 p0 = pMesh->_positions[v0.positionIndex];
			Vec3 p1 = pMesh->_positions[v1.positionIndex];
			Vec3 p2 = pMesh->_positions[v2.positionIndex];

			Vec3 normal = (p1 - p0).Cross(p2 - p0);
			normal.Norm();

			pMesh->_normals[v0.positionIndex] += normal;
			pMesh->_normals[v1.positionIndex] += normal;
			pMesh->_normals[v2.positionIndex] += normal;

			v0.normalIndex = v0.positionIndex;
			v1.normalIndex = v1.positionIndex;
			v2.normalIndex = v2.positionIndex;
		}

		for (uint32_t i = 0; i < pMesh->_normals.size(); i++)
		{
			pMesh->_normals[i].Norm();
		}
	}

	void Importer::ComputeTangents(MeshInternalData* pMesh)
	{
		pMesh->_tangents.resize(pMesh->_positions.size());
		if (pMesh->_texCoords.size())
		{
			for (uint32_t i = 0; i < pMesh->_polyVerts.size(); i += 3)
			{
				PolygonVertex& v0 = pMesh->_polyVerts[i + 0];
				PolygonVertex& v1 = pMesh->_polyVerts[i + 1];
				PolygonVertex& v2 = pMesh->_polyVerts[i + 2];

				Vec3 p0 = pMesh->_positions[v0.positionIndex];
				Vec3 p1 = pMesh->_positions[v1.positionIndex];
				Vec3 p2 = pMesh->_positions[v2.positionIndex];

				if (v0.texCoordIndex != -1 && v1.texCoordIndex != -1 && v2.texCoordIndex != -1)
				{
					Vec2 tc0 = pMesh->_texCoords[v0.texCoordIndex];
					Vec2 tc1 = pMesh->_texCoords[v1.texCoordIndex];
					Vec2 tc2 = pMesh->_texCoords[v2.texCoordIndex];

					Vec3 e1 = p1 - p0;
					Vec3 e2 = p2 - p0;

					Vec2 et1 = tc1 - tc0;
					Vec2 et2 = tc2 - tc0;

					float det = et1.x * et2.y - et1.y * et2.x;
					if (det != 0.0f) det = 1.0f / det;

					//[d, -b]
					//[-c, a]
					float mtx00 = et2.y * det;
					float mtx01 = -et1.y * det;
					//float mtx10 = et1.x * det;
					//float mtx11 = -et2.y * det;

					Vec3 tangent;
					tangent.x = mtx00 * e1.x + mtx01 * e2.x;
					tangent.y = mtx00 * e1.y + mtx01 * e2.y;
					tangent.z = mtx00 * e1.z + mtx01 * e2.z;
					tangent.Norm();

					pMesh->_tangents[v0.positionIndex] += tangent;
					pMesh->_tangents[v1.positionIndex] += tangent;
					pMesh->_tangents[v2.positionIndex] += tangent;
				}
			}
		}

		for (uint32_t i = 0; i < pMesh->_tangents.size(); i++)
		{
			if (pMesh->_tangents[i].GetLengthSquared() > 0.0f)
			{
				pMesh->_tangents[i].Norm();
			}
			//else
			//{
			//	Vec3 norm = pMesh->_normals[pMesh->_polyVerts[i].normalIndex];
			//	pMesh->_tangents[i] = Vec3(0, 0, 0);
			//}
		}
	}

	void Importer::MakeTwoSided(MeshData * pMesh)
	{
		//TODO?
		if (pMesh->VertexBones.size())
			return;

		std::vector<Vertex> prevVerts = pMesh->Vertices;
		std::vector<uint32_t> prevIndices = pMesh->Indices;

		pMesh->Vertices.resize(prevVerts.size() * 2);
		pMesh->Indices.resize(prevIndices.size() * 2);

		memcpy(pMesh->Vertices.data(), prevVerts.data(), sizeof(Vertex) * prevVerts.size());
		memcpy(pMesh->Indices.data(), prevIndices.data(), sizeof(uint32_t) * prevIndices.size());

		for (uint32_t i = 0; i < prevVerts.size(); i++)
		{
			Vertex vert = pMesh->Vertices[i];
			vert.normal *= -1.0f;

			pMesh->Vertices[i + prevVerts.size()] = vert;
		}

		for (uint32_t i = 0; i < prevIndices.size(); i+=3)
		{
			uint32_t i0 = prevIndices[i + 0] + prevVerts.size();
			uint32_t i1 = prevIndices[i + 1] + prevVerts.size();
			uint32_t i2 = prevIndices[i + 2] + prevVerts.size();

			pMesh->Indices[i + prevIndices.size() + 0] = i0;
			pMesh->Indices[i + prevIndices.size() + 1] = i2;
			pMesh->Indices[i + prevIndices.size() + 2] = i1;
		}
	}

	bool Importer::IsBonePathRemoveable(Node* pNode, uint32_t* pBoneUsageCheck) const
	{
		if (pNode->GetType() == MESH)
			return false;

		if (pNode->GetType() == BONE && pBoneUsageCheck[pNode->GetID()] > 0)
			return false;

		for (auto iter = pNode->Children.begin(); iter != pNode->Children.end(); ++iter)
		{
			if (!IsBonePathRemoveable((*iter), pBoneUsageCheck))
				return false;
		}

		return true;
	}

	void Importer::RemoveNodes(std::list<Node*>& nodeRemoveList)
	{
		std::vector<Node*> deleteList;

		while (nodeRemoveList.size())
		{
			std::vector<Node*> pathList;
			nodeRemoveList.front()->CollectNodes(pathList);

			for (uint32_t i = 0; i < pathList.size(); i++)
			{
				Node* pNode = pathList[i];

				if (pNode->GetType() == MESH)
				{
					Meshes[pNode->GetID()] = NULL;
				}
				else if (pNode->GetType() == BONE)
				{
					Bones[pNode->GetID()] = NULL;
				}

				for (uint32_t j = 0; j < Nodes.size(); j++)
				{
					if (Nodes[j] == pNode)
					{
						Nodes[j] = 0;
						break;
					}
				}

				if (pNode->Parent)
					pNode->Detach();

				nodeRemoveList.remove(pNode);
			}
			deleteList.insert(deleteList.end(), pathList.begin(), pathList.end());
		}

		if (deleteList.size())
		{
			for (uint32_t i = 0; i < deleteList.size(); i++)
			{
				delete deleteList[i];
			}

			std::vector<Mesh*> validMeshes;
			for (uint32_t i = 0; i < Meshes.size(); i++)
			{
				if (Meshes[i])
					validMeshes.push_back(Meshes[i]);

				static_cast<ISerialize*>(Meshes[i])->id = i;
			}
			Meshes = validMeshes;

			std::vector<Bone*> validBones;
			for (uint32_t i = 0; i < Bones.size(); i++)
			{
				if (Bones[i])
					validBones.push_back(Bones[i]);
			}
			bool bonesReduced = validBones.size() < Bones.size();
			Bones = validBones;

			if (bonesReduced)
			{
				for (uint32_t i = 0; i < Animations.size(); i++)
				{
					for (uint32_t j = 0; j < Animations[i]->KeyFrames.size(); j++)
					{
						KeyFrame& kf = Animations[i]->KeyFrames[j];
						std::vector<BoneTransform> newTransforms;
						newTransforms.resize(Bones.size());

						for (uint32_t b = 0; b < Bones.size(); b++)
						{
							newTransforms[b] = kf.Transforms[Bones[b]->GetID()];
						}

						kf.Transforms = newTransforms;
					}
				}

				for (uint32_t i = 0; i < Bones.size(); i++)
				{
					for (uint32_t j = 0; j < MeshDatas.size(); j++)
					{
						for (uint32_t v = 0; v < MeshDatas[j]->VertexBones.size(); v++)
						{
							uint32_t b = 0;
							while (b < VertexBoneInfo::MAX_VERTEX_BONES && MeshDatas[j]->VertexBones[v].weights[b] > 0.0f)
							{
								if ((uint32_t)MeshDatas[j]->VertexBones[v].bones[b] == Bones[i]->GetID())
								{
									MeshDatas[j]->VertexBones[v].bones[b] = (float)i;
								}
								b++;
							}
						}
					}

					static_cast<ISerialize*>(Bones[i])->id = i;
				}
			}

			std::vector<Node*> validNodes;
			for (uint32_t i = 0; i < Nodes.size(); i++)
			{
				if (Nodes[i])
					validNodes.push_back(Nodes[i]);
			}
			Nodes = validNodes;
		}

	}

	Importer::~Importer()
	{
		Destroy();
	}

	Importer::FileStream::FileStream()
	{
		_handle = 0;
	}

	Importer::FileStream::~FileStream()
	{
		Close();
	}

	bool Importer::FileStream::OpenForWrite(const char* filename)
	{
		if (_handle)
			Close();

		return fopen_s(&_handle, filename, "wb") == 0;
	}

	bool Importer::FileStream::OpenForRead(const char* filename)
	{
		if (_handle)
			Close();

		return fopen_s(&_handle, filename, "rb") == 0;
	}

	bool Importer::FileStream::Close()
	{
		if (_handle == 0)
			return true;

		int closed = fclose(_handle);
		_handle = 0;
		return closed == 0;
	}
		
	void Importer::FileStream::Write(const void* pData, uint32_t size)
	{
		fwrite(pData, size, 1, _handle);
	}

	void Importer::FileStream::Read(void* pData, const uint32_t size)
	{
		fread(pData, size, 1, _handle);
	}

	void Importer::FileStream::Write(const std::string& str)
	{
		Write((uint32_t)str.size());
		Write(str.data(), str.size());
	}

	void Importer::FileStream::Read(std::string& str)
	{
		uint32_t size;
		Read(size);
		str.resize(size);
		Read(&str[0], size);
	}
	
	void Importer::FileStream::Write(uint32_t value)
	{
		fwrite(&value, sizeof(uint32_t), 1, _handle);
	}

	void Importer::FileStream::Read(uint32_t& value)
	{
		fread(&value, sizeof(uint32_t), 1, _handle);
	}
		
	void Importer::FileStream::Write(float value)
	{
		fwrite(&value, sizeof(float), 1, _handle);
	}

	void Importer::FileStream::Read(float& value)
	{
		fread(&value, sizeof(float), 1, _handle);
	}
		
	void Importer::FileStream::Write(double value)
	{
		fwrite(&value, sizeof(double), 1, _handle);
	}

	void Importer::FileStream::Read(double& value)
	{
		fread(&value, sizeof(double), 1, _handle);
	}

	bool Importer::Save(const std::string & filename)
	{
		FileStream stream;	
		if (!stream.OpenForWrite(filename.data()))
			return false;		

		stream.Write(MODEL_FILE_KEY);

		stream.Write(_fileName);

		stream.Write((uint32_t)Textures.size());
		for (uint32_t i = 0; i < Textures.size(); i++)
			Textures[i]->Write(stream, this);

		stream.Write((uint32_t)Materials.size());
		for (uint32_t i = 0; i < Materials.size(); i++)
			Materials[i]->Write(stream, this);

		stream.Write((uint32_t)Animations.size());
		for (uint32_t i = 0; i < Animations.size(); i++)
			Animations[i]->Write(stream, this);

		stream.Write((uint32_t)Bones.size());
		for (uint32_t i = 0; i < Bones.size(); i++)
			Bones[i]->Write(stream, this);

		stream.Write((uint32_t)Meshes.size());
		for (uint32_t i = 0; i < Meshes.size(); i++)
			Meshes[i]->Write(stream, this);

		stream.Close();
		return true;
	}

	bool Importer::Load(const std::string & filename)
	{
		FileStream stream;
		if (!stream.OpenForRead(filename.data()))
			return false;

		uint32_t key;
		stream.Read(&key, sizeof(key));
		if (key != MODEL_FILE_KEY)
		{
			stream.Close();
			return false;
		}

		stream.Read(_fileName);

		uint32_t count;

		stream.Read(count);
		Textures.resize(count);
		for (uint32_t i = 0; i < count; i++)
			(new Texture(ISerialize::INVALID_ID))->Read(stream, this);

		stream.Read(count);
		Materials.resize(count);
		for (uint32_t i = 0; i < count; i++)
			(new Material(ISerialize::INVALID_ID))->Read(stream, this);

		stream.Read(count);
		Animations.resize(count);
		for (uint32_t i = 0; i < count; i++)
			(new Animation(ISerialize::INVALID_ID))->Read(stream, this);

		stream.Read(count);
		Bones.resize(count);
		for (uint32_t i = 0; i < count; i++)
			(new Bone(ISerialize::INVALID_ID))->Read(stream, this);

		stream.Read(count);
		Meshes.resize(count);
		for (uint32_t i = 0; i < count; i++)
			(new Mesh(ISerialize::INVALID_ID))->Read(stream, this);

		//Bone fixup...
		//for (bones* pBone : Bones)
		//{
		//	uint32_t id;

		//	id = (uint32_t)pBone->Parent;
		//	if (id != ISerialize::INVALID_ID)
		//		pBone->Parent = Bones[id];

		//	std::list<Bone*> remappedChildren;
		//	for (Bone* pChild : pBone->Children)
		//	{
		//		id = (uint32_t)pChild;
		//		remappedChildren.push_back(Bones[id]);
		//	}

		//	pBone->Children = remappedChildren;
		//}

		stream.Close();
		return true;
	}

	Importer::ISerialize::ISerialize(uint32_t _id)
	{
		id = _id;
	}

	void Importer::ISerialize::Write(FileStream& stream, Importer* pImporter)
	{
		stream.Write(id);
	}

	void Importer::ISerialize::Read(FileStream& stream, Importer* pImporter)
	{
		stream.Read(id);

		ISerialize* pThis = this;
		if (dynamic_cast<Texture*>(pThis))
		{
			pImporter->Textures[id] = static_cast<Texture*>(pThis);
		}
		else if (dynamic_cast<Material*>(pThis))
		{
			pImporter->Materials[id] = static_cast<Material*>(pThis);
		}
		else if (dynamic_cast<Animation*>(pThis))
		{
			pImporter->Animations[id] = static_cast<Animation*>(pThis);
		}
		else if (dynamic_cast<Bone*>(pThis))
		{
			pImporter->Bones[id] = static_cast<Bone*>(pThis);
		}
		else if (dynamic_cast<Mesh*>(pThis))
		{
			pImporter->Meshes[id] = static_cast<Mesh*>(pThis);
		}
	}

	Importer::Texture::Texture(uint32_t id) : ISerialize(id)
	{
	}

	//void Importer::Texture::Write(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Write(stream, pImporter);

	//	stream.Write(FileName);
	//}

	//void Importer::Texture::Read(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Read(stream, pImporter);

	//	stream.Read(FileName);
	//}

	Importer::Material::Material(uint32_t id) : ISerialize(id)
	{
		DiffuseColor.Set(1);
		SpecularColor.Set(0);
		SpecularExponent = 1.0f;
		Alpha = 1.0f;

		DiffuseMap = 0;
		NormalMap = 0;
		SpecularMap = 0;
		TransparentMap = 0;
	}

	//void Importer::Material::Write(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Write(stream, pImporter);

	//	stream.Write(Name);

	//	stream.Write(&DiffuseColor, sizeof(DiffuseColor));
	//	stream.Write(Alpha);
	//	stream.Write(&SpecularColor, sizeof(SpecularColor));
	//	stream.Write(SpecularExponent);

	//	stream.Write((DiffuseMap ? DiffuseMap->GetID() : INVALID_ID));
	//	stream.Write((NormalMap ? NormalMap->GetID() : INVALID_ID));
	//}

	//void Importer::Material::Read(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Read(stream, pImporter);

	//	stream.Read(Name);

	//	stream.Read(&DiffuseColor, sizeof(DiffuseColor));
	//	stream.Read(Alpha);
	//	stream.Read(&SpecularColor, sizeof(SpecularColor));
	//	stream.Read(SpecularExponent);

	//	uint32_t id;

	//	stream.Read(id); if (id != INVALID_ID) DiffuseMap = pImporter->Textures[id];
	//	stream.Read(id); if (id != INVALID_ID) NormalMap = pImporter->Textures[id];
	//}

	Importer::Animation::Animation(uint32_t id) : ISerialize(id)
	{
		pUserData = 0;
	}

	//void Importer::Animation::Write(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Write(stream, pImporter);

	//	stream.Read(Name);

	//	stream.Write(Length);

	//	stream.Write((uint32_t)KeyFrames.size());
	//	for (KeyFrame& kf : KeyFrames)
	//	{
	//		stream.Write(kf.Time);
	//		stream.Write((uint32_t)kf.Transforms.size());
	//		stream.Write(kf.Transforms.data(), sizeof(BoneTransform) * kf.Transforms.size());
	//	}
	//}

	//void Importer::Animation::Read(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Read(stream, pImporter);

	//	stream.Read(Name);

	//	stream.Read(Length);

	//	uint32_t kfCount;
	//	stream.Read(kfCount);
	//	KeyFrames.resize(kfCount);
	//	for(uint32_t i = 0; i < kfCount; i++)
	//	{
	//		KeyFrame& kf = KeyFrames[i];
	//		uint32_t btCount;

	//		stream.Read(kf.Time);
	//		stream.Read(btCount);
	//		kf.Transforms.resize(btCount);
	//		stream.Read(kf.Transforms.data(), sizeof(BoneTransform) * kf.Transforms.size());
	//	}
	//}

	Importer::Bone::Bone(uint32_t id) : Node(BONE, id)
	{
		pUserData = 0;
		Parent = 0;
	}

	//void Importer::Bone::Write(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Write(stream, pImporter);

	//	stream.Write(Name);

	//	stream.Write(&WorldMatrix, sizeof(WorldMatrix));
	//	stream.Write(&Color, sizeof(Color));
	//	stream.Write((Parent ? Parent->GetID() : INVALID_ID));

	//	stream.Write((uint32_t)SkinMatrices.size());
	//	stream.Write(SkinMatrices.data(), sizeof(Mat4) * SkinMatrices.size());

	//	stream.Write((uint32_t)Children.size());
	//	for (Bone* child : Children)
	//	{
	//		stream.Write(child->GetID());
	//	}
	//}

	//void Importer::Bone::Read(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Read(stream, pImporter);

	//	stream.Read(Name);

	//	stream.Read(&WorldMatrix, sizeof(WorldMatrix));
	//	stream.Read(&Color, sizeof(Color));

	//	uint32_t pid;
	//	stream.Read(pid);
	//	Parent = (Bone*)pid;

	//	uint32_t sCount = 0;
	//	stream.Read(sCount);
	//	SkinMatrices.resize(sCount);
	//	stream.Read(SkinMatrices.data(), sizeof(Mat4) * SkinMatrices.size());

	//	uint32_t cCount = 0;
	//	stream.Read(cCount);
	//	for(uint32_t i = 0; i < cCount; i++)
	//	{
	//		uint32_t cid;
	//		stream.Read(cid);
	//		Children.push_back((Bone*)cid);
	//	}
	//}

	Importer::Mesh::Mesh(uint32_t id) : Node(MESH, id)
	{
		MeshData = 0;
		Material = 0;
	}

	//void Importer::Mesh::Write(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Write(stream, pImporter);

	//	stream.Write(Name);

	//	stream.Write((uint32_t)Vertices.size());
	//	stream.Write(Vertices.data(), sizeof(Vertex) * Vertices.size());

	//	stream.Write((uint32_t)VertexBones.size());
	//	stream.Write(VertexBones.data(), sizeof(VertexBoneInfo) * VertexBones.size());

	//	stream.Write((uint32_t)Indices.size());
	//	stream.Write(Indices.data(), sizeof(uint32_t) * Indices.size());

	//	stream.Write((Material ? Material->GetID() : INVALID_ID));

	//	stream.Write(&BoundingBox, sizeof(BoundingBox));
	//	stream.Write(&LocalMatrix, sizeof(LocalMatrix));
	//	stream.Write(&WorldMatrix, sizeof(WorldMatrix));
	//	stream.Write(&GeometryOffset, sizeof(GeometryOffset));

	//	stream.Write(SkinIndex);

	//	stream.Write((ParentBone ? ParentBone->GetID() : INVALID_ID));
	//}

	//void Importer::Mesh::Read(FileStream& stream, Importer* pImporter)
	//{
	//	ISerialize::Read(stream, pImporter);

	//	uint32_t count;

	//	stream.Read(Name);

	//	stream.Read(count);
	//	Vertices.resize(count);
	//	stream.Read(Vertices.data(), sizeof(Vertex) * Vertices.size());

	//	stream.Read(count);
	//	VertexBones.resize(count);
	//	stream.Read(VertexBones.data(), sizeof(VertexBoneInfo) * VertexBones.size());

	//	stream.Read(count);
	//	Indices.resize(count);
	//	stream.Read(Indices.data(), sizeof(uint32_t) * Indices.size());

	//	uint32_t id;
	//	stream.Read(id); if (id != INVALID_ID) Material = pImporter->Materials[id];

	//	stream.Read(&BoundingBox, sizeof(BoundingBox));
	//	stream.Read(&LocalMatrix, sizeof(LocalMatrix));
	//	stream.Read(&WorldMatrix, sizeof(WorldMatrix));
	//	stream.Read(&GeometryOffset, sizeof(GeometryOffset));

	//	stream.Read(SkinIndex);

	//	stream.Read(id); if (id != INVALID_ID) ParentBone = pImporter->Bones[id];
	//}


	Importer::Node::Node(NodeType type, uint32_t id) : ISerialize(id)
	{
		Type = type;
		Parent = 0;
	}

	void Importer::Node::Attach(Node* pChild)
	{
		if (pChild->Parent)
			pChild->Parent->Children.remove(pChild);

		pChild->Parent = this;
		Children.push_back(pChild);
	}

	void Importer::Node::Detach()
	{
		if (Parent)
			Parent->Children.remove(this);

		Parent = 0;
	}

	void Importer::Node::CollectNodes(std::vector<Node*>& list)
	{
		list.push_back(this);

		for (auto iter = Children.begin(); iter != Children.end(); ++iter)
		{
			(*iter)->CollectNodes(list);
		}
	}

	//void Importer::Node::Write(FileStream& stream, Importer* pImporter)
	//{
	//}

	//void Importer::Node::Read(FileStream& stream, Importer* pImporter)
	//{
	//}

}
