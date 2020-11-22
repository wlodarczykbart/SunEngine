#include <fstream>

#include "OBJImporter.h"

namespace ModelImporter
{

	enum ObjVertType
	{
		OV_POS,
		OV_POS_NORM,
		OV_POS_TEXCOORD,
		OV_POS_TEXCOORD_NORM,
	};

	OBJImporter::OBJImporter()
	{
		_processFuncs["v"] = &OBJImporter::ProcessVertexPosition;
		_processFuncs["vt"] = &OBJImporter::ProcessVertexTexCoord;
		_processFuncs["vn"] = &OBJImporter::ProcessVertexNormal;
		_processFuncs["f"] = &OBJImporter::ProcessFace;
		_processFuncs["usemtl"] = &OBJImporter::ProcessUseMtl;
		_processFuncs["o"] = &OBJImporter::ProcessObjectName;
		_processFuncs["g"] = &OBJImporter::ProcessGroupName;
		_processFuncs["mtllib"] = &OBJImporter::ProcessMtlLib;

		_processMatFuncs["newmtl"] = &OBJImporter::ProcessNewMaterial;
		_processMatFuncs["Kd"] = &OBJImporter::ProcessDiffuseColor;
		_processMatFuncs["Ks"] = &OBJImporter::ProcessSpecularColor; 
		_processMatFuncs["Ns"] = &OBJImporter::ProcessSpecularExponent;
		_processMatFuncs["map_Kd"] = &OBJImporter::ProcessDiffuseTexture;
		_processMatFuncs["map_bump"] = &OBJImporter::ProcessNormalMapTexture;
		_processMatFuncs["bump"] = &OBJImporter::ProcessNormalMapTexture;
		_processMatFuncs["d"] = &OBJImporter::ProcessDissolved;
		_processMatFuncs["Tr"] = &OBJImporter::ProcessTransparency;

		_offsetVertex.positionIndex = 0;
		_offsetVertex.texCoordIndex = 0;
		_offsetVertex.normalIndex = 0;
	}


	OBJImporter::~OBJImporter()
	{
	}

	Importer::MeshInternalData * OBJImporter::GetCurrentMesh()
	{
		if ((_currentType == "v" && _previousType != "v") )
		{
			FinishMesh();
		}

		return &_meshInfo.currentMesh;
	}

	Importer::Material * OBJImporter::GetCurrentMaterial()
	{
		if (GetMaterialCount())
			return GetMaterial(GetMaterialCount() - 1);
		else
			return 0;

	}

	bool OBJImporter::DerivedImport()
	{
		std::ifstream fstream;
		fstream.open(_fileName);

		if (!fstream.is_open())
			return false;

		std::string objText = std::string(std::istreambuf_iterator<char>(fstream), std::istreambuf_iterator<char>(0));

		objText = StrUtil::Remove(objText, '\r');

		std::vector<std::string> objLines;
		StrUtil::Split(objText, objLines, '\n');

		//Normal Pass
		for (size_t i = 0; i < objLines.size(); i++)
		{
			std::string &line = objLines[i];
			size_t spacePos = line.find(' ');
			if (spacePos != std::string::npos) 
			{
				_currentType = line.substr(0, spacePos);
				std::unordered_map<std::string, ProcessLine>::iterator funcIter = _processFuncs.find(_currentType);
				if (funcIter != _processFuncs.end())
				{
					ProcessLine func = (*funcIter).second;
					(this->*func)(line.substr(spacePos + 1), GetCurrentMesh());
				}
				_previousType = _currentType;
				_meshInfo.Update();
			}
		}
		FinishMesh();



		fstream.close();
		return true;
	}

	void OBJImporter::ProcessVertexPosition(const std::string& line, MeshInternalData* pMesh)
	{
		std::vector<std::string> parts;
		StrUtil::Split(line, parts, ' ');
		Vec3 data;
		data.x = StrUtil::ToFloat(parts[0]);
		data.y = StrUtil::ToFloat(parts[1]);
		data.z = StrUtil::ToFloat(parts[2]);
		pMesh->_positions.push_back(data);
	}

	void OBJImporter::ProcessVertexNormal(const std::string& line, MeshInternalData* pMesh)
	{
		std::vector<std::string> parts;
		StrUtil::Split(line, parts, ' ');
		Vec3 data;
		data.x = StrUtil::ToFloat(parts[0]);
		data.y = StrUtil::ToFloat(parts[1]);
		data.z = StrUtil::ToFloat(parts[2]);
		data.Norm();
		pMesh->_normals.push_back(data);
	}

	void OBJImporter::ProcessVertexTexCoord(const std::string& line, MeshInternalData* pMesh)
	{
		std::vector<std::string> parts;
		StrUtil::Split(line, parts, ' ');
		Vec2 data;
		data.x = StrUtil::ToFloat(parts[0]);
		data.y = 1.0f - StrUtil::ToFloat(parts[1]);
		pMesh->_texCoords.push_back(data);
	}

	void OBJImporter::ProcessFace(const std::string& line, MeshInternalData* pMesh) 
	{
		static PolygonVertex localVerts[128];

		std::vector<std::string> parts;
		StrUtil::Split(line, parts, ' ');

		if (parts.size() > 4)
		{
			printf("Unsupported Polygon with %d components\n", (int)parts.size());
			return;
		}

		std::string& vertStr = parts[0];

		ObjVertType type = OV_POS;;
		uint32_t slashCount = 0;
		for (uint32_t i = 0; i < vertStr.size(); i++)
		{
			if (vertStr[i] == '/')
			{
				if (vertStr[i + 1] == '/')
				{
					type = OV_POS_NORM;
					break;
				}

				slashCount++;
				if (slashCount == 2)
					type = OV_POS_TEXCOORD_NORM;
				else
					type = OV_POS_TEXCOORD;

			}
		}
		

		for (uint32_t i = 0; i < parts.size(); i++)
		{
			PolygonVertex vert;

			switch (type)
			{
			case OV_POS:
				vert.positionIndex = StrUtil::ToInt(parts[i]);
				break;
			case OV_POS_NORM:
			{
				size_t slashPos = parts[i].find('/');
				vert.positionIndex = StrUtil::ToInt(parts[i].substr(0, slashPos));
				vert.normalIndex = StrUtil::ToInt(parts[i].substr(slashPos + 2));
			}
				break;
			case OV_POS_TEXCOORD:
			{
				size_t slashPos = parts[i].find('/');
				vert.positionIndex = StrUtil::ToInt(parts[i].substr(0, slashPos));
				vert.texCoordIndex = StrUtil::ToInt(parts[i].substr(slashPos + 1));
			}
				break;
			case OV_POS_TEXCOORD_NORM:
			{
				size_t slashPos = parts[i].find('/');
				vert.positionIndex = StrUtil::ToInt(parts[i].substr(0, slashPos));
				vert.texCoordIndex = StrUtil::ToInt(parts[i].substr(slashPos + 1));
				slashPos = parts[i].find('/', slashPos + 1);
				vert.normalIndex = StrUtil::ToInt(parts[i].substr(slashPos + 1));
			}
				break;
			default:
				break;
			}

			localVerts[i] = vert;
		}

		if (parts.size() == 3)
		{
			pMesh->_polyVerts.push_back(localVerts[0]);
			pMesh->_polyVerts.push_back(localVerts[1]);
			pMesh->_polyVerts.push_back(localVerts[2]);
		}
		else if (parts.size() == 4)
		{
			pMesh->_polyVerts.push_back(localVerts[0]);
			pMesh->_polyVerts.push_back(localVerts[1]);
			pMesh->_polyVerts.push_back(localVerts[2]);

			pMesh->_polyVerts.push_back(localVerts[0]);
			pMesh->_polyVerts.push_back(localVerts[2]);
			pMesh->_polyVerts.push_back(localVerts[3]);
		}


	}

	void OBJImporter::ProcessMtlLib(const std::string & line, MeshInternalData * pMesh)
	{
		(void)pMesh;
		ImportMaterialFile(line);
	}

	void OBJImporter::ProcessUseMtl(const std::string& line, MeshInternalData* pMesh)
	{
		(void)pMesh;
		_meshInfo.materialName = line;
	}

	void OBJImporter::ProcessGroupName(const std::string& line, MeshInternalData* pMesh)
	{
		(void)pMesh;
		_meshInfo.groupName = line;
	}

	void OBJImporter::ProcessObjectName(const std::string& line, MeshInternalData* pMesh)
	{
		(void)pMesh;
		_meshInfo.objectName = line;
	}

	void OBJImporter::ProcessNewMaterial(const std::string & line, Material * pMaterial)
	{
		(void)pMaterial;
		Material* pMat = AddMaterial();
		pMat->Name = line;
	}

	void OBJImporter::ProcessDiffuseColor(const std::string & line, Material * pMaterial)
	{
		std::vector<std::string> parts;
		StrUtil::Split(line, parts, ' ');
		pMaterial->DiffuseColor.Set(StrUtil::ToFloat(parts[0]), StrUtil::ToFloat(parts[1]), StrUtil::ToFloat(parts[2]));
	}

	void OBJImporter::ProcessSpecularColor(const std::string & line, Material * pMaterial)
	{		std::vector<std::string> parts;
		StrUtil::Split(line, parts, ' ');
		pMaterial->SpecularColor.Set(StrUtil::ToFloat(parts[0]), StrUtil::ToFloat(parts[1]), StrUtil::ToFloat(parts[2]));
	}

	void OBJImporter::ProcessSpecularExponent(const std::string & line, Material * pMaterial)
	{
		pMaterial->SpecularExponent = StrUtil::ToFloat(line);
	}

	void OBJImporter::ProcessDiffuseTexture(const std::string & line, Material * pMaterial)
	{
		std::string texFile;
		if (GetFullFilePath(line, texFile))
			pMaterial->DiffuseMap = GetTexture(texFile);
	}

	void OBJImporter::ProcessNormalMapTexture(const std::string & line, Material * pMaterial)
	{
		std::string texFile;
		if (GetFullFilePath(line, texFile))
			pMaterial->NormalMap = GetTexture(texFile);
	}

	void OBJImporter::ProcessDissolved(const std::string & line, Material * pMaterial)
	{
		pMaterial->Alpha = StrUtil::ToFloat(line);
	}

	void OBJImporter::ProcessTransparency(const std::string & line, Material * pMaterial)
	{
		pMaterial->Alpha = 1.0f - StrUtil::ToFloat(line);
	}

	void OBJImporter::FinishMesh()
	{
		_meshInfo.Update();

		if (_meshInfo.currentMesh._polyVerts.size())
		{
			MeshInternalData* pMesh = &_meshInfo.currentMesh;
			for (uint32_t i = 0; i < pMesh->_polyVerts.size(); i++)
			{
				pMesh->_polyVerts[i].positionIndex = pMesh->_polyVerts[i].positionIndex - 1 - _offsetVertex.positionIndex;
				if (pMesh->_polyVerts[i].texCoordIndex != -1)
					pMesh->_polyVerts[i].texCoordIndex = pMesh->_polyVerts[i].texCoordIndex - 1 - _offsetVertex.texCoordIndex;
				if (pMesh->_polyVerts[i].normalIndex != -1)
					pMesh->_polyVerts[i].normalIndex = pMesh->_polyVerts[i].normalIndex - 1 - _offsetVertex.normalIndex;
			}

			_offsetVertex.positionIndex += (int)pMesh->_positions.size();
			_offsetVertex.normalIndex += (int)pMesh->_normals.size();
			_offsetVertex.texCoordIndex += (int)pMesh->_texCoords.size();

			if (_meshInfo.materials.size() == 0)
			{
				Mesh* pNewMesh = AddMesh();
				pNewMesh->Name = _meshInfo.meshName;
				pNewMesh->pUserData = new MeshInternalData(_meshInfo.currentMesh);
			}
			else
			{
				std::vector<MaterialInfo>& materials = _meshInfo.materials;

				for (uint32_t i = 0; i < materials.size(); i++)
				{
					std::string& matName = materials[i].name;
					uint32_t polyStart = materials[i].polyVertStart;
					size_t polyEnd = (i != materials.size() - 1) ? materials[i + 1].polyVertStart : pMesh->_polyVerts.size();

					if (polyEnd != 0)
					{
						Mesh* pNewMesh = AddMesh();
						pNewMesh->Name = _meshInfo.meshName + "_" + matName;
						pNewMesh->Material = GetMaterial(matName);

						MeshInternalData* pNewMeshData = new MeshInternalData();
						pNewMeshData->_polyVerts.resize(polyEnd - polyStart);
						memcpy(pNewMeshData->_polyVerts.data(), &pMesh->_polyVerts[polyStart], pNewMeshData->_polyVerts.size() * sizeof(PolygonVertex));
						RemapMesh(pNewMeshData, pMesh->_positions, pMesh->_normals, pMesh->_texCoords);
						pNewMesh->pUserData = pNewMeshData;
					}
				}
			}
		}

		_meshInfo.Reset();
	}

	void OBJImporter::ImportMaterialFile(const std::string & name)
	{
		std::string matFile = name;
		matFile = StrUtil::GetFileName(matFile);
		matFile = StrUtil::GetDirectory(_fileName) + "/" + matFile;

		std::ifstream fstream;
		fstream.open(matFile);

		if (!fstream.is_open())
			return;

		std::string matText = std::string(std::istreambuf_iterator<char>(fstream), std::istreambuf_iterator<char>());

		matText = StrUtil::Remove(matText, '\r');
		matText = StrUtil::Remove(matText, '\t');

		std::vector<std::string> matLines;
		StrUtil::Split(matText, matLines, '\n');

		for (uint32_t i = 0; i < matLines.size(); i++)
		{
			std::string line = StrUtil::TrimStart(matLines[i]);
			size_t spacePos = line.find(' ');
			if (spacePos != std::string::npos) 
			{
				std::string key = line.substr(0, spacePos);
				std::unordered_map<std::string, ProcessMatLine>::iterator funcIter = _processMatFuncs.find(key);
				if (funcIter != _processMatFuncs.end())
				{
					ProcessMatLine func = (*funcIter).second;
					std::string value = line.substr(spacePos + 1);
					(this->*func)(value, GetCurrentMaterial());
				}
			}
		}

	}

	void OBJImporter::RemapMesh(MeshInternalData * pMesh, std::vector<Vec3>& positions, std::vector<Vec3>& normals, std::vector<Vec2>& texCoords)
	{

		std::unordered_map<uint32_t, uint32_t> positionRemap;
		std::unordered_map<uint32_t, uint32_t> normalRemap;
		std::unordered_map<uint32_t, uint32_t> texCoordRemap;

		for (uint32_t i = 0; i < pMesh->_polyVerts.size(); i++)
		{
			PolygonVertex& vert = pMesh->_polyVerts[i];

			std::unordered_map<uint32_t, uint32_t>::iterator posIter = positionRemap.find(vert.positionIndex);
			if (posIter == positionRemap.end())
			{
				positionRemap[vert.positionIndex] = (uint32_t)positionRemap.size();
				vert.positionIndex = (int)positionRemap.size() - 1;
			}
			else
			{
				vert.positionIndex = (int)(*posIter).second;
			}

			if (vert.normalIndex != -1)
			{
				std::unordered_map<uint32_t, uint32_t>::iterator normIter = normalRemap.find(vert.normalIndex);
				if (normIter == normalRemap.end())
				{
					normalRemap[vert.normalIndex] = (uint32_t)normalRemap.size();
					vert.normalIndex = (int)normalRemap.size() - 1;
				}
				else
				{
					vert.normalIndex = (int)(*normIter).second;
				}
			}

			if (vert.texCoordIndex != -1)
			{
				std::unordered_map<uint32_t, uint32_t>::iterator texCoordIter = texCoordRemap.find(vert.texCoordIndex);
				if (texCoordIter == texCoordRemap.end())
				{
					texCoordRemap[vert.texCoordIndex] = (uint32_t)texCoordRemap.size();
					vert.texCoordIndex = (int)texCoordRemap.size() - 1;
				}
				else
				{
					vert.texCoordIndex = (int)(*texCoordIter).second;
				}
			}
		}

		pMesh->_positions.resize(positionRemap.size());
		pMesh->_normals.resize(normalRemap.size());
		pMesh->_texCoords.resize(texCoordRemap.size());

		std::unordered_map<uint32_t, uint32_t>::iterator posIter = positionRemap.begin();
		while (posIter != positionRemap.end())
		{
			pMesh->_positions[(*posIter).second] = positions[(*posIter).first];
			posIter++;
		}

		std::unordered_map<uint32_t, uint32_t>::iterator normIter = normalRemap.begin();
		while (normIter != normalRemap.end())
		{
			pMesh->_normals[(*normIter).second] = normals[(*normIter).first];
			normIter++;
		}

		std::unordered_map<uint32_t, uint32_t>::iterator texCoordIter = texCoordRemap.begin();
		while (texCoordIter != texCoordRemap.end())
		{
			pMesh->_texCoords[(*texCoordIter).second] = texCoords[(*texCoordIter).first];
			texCoordIter++;
		}
	}

	OBJImporter::MeshInfo::MeshInfo()
	{
	}

	void OBJImporter::MeshInfo::Update()
	{
		if (objectName.size() && meshName.size() == 0)
		{
			meshName = objectName;
			objectName = "";
		}

		if (groupName.size() && meshName.size() == 0)
		{
			meshName = groupName;
			groupName = "";
		}

		if (materialName.size() && (materials.size() == 0 || materials.back().name != materialName))
		{
			//this mesh will need to be seperated at FinishMesh, we need to start poly vertex index to know where to seperate it...
			MaterialInfo info;
			info.name = materialName;
			info.polyVertStart = (uint32_t)currentMesh._polyVerts.size();//TODO + 1?
			materials.push_back(info);
			materialName = "";
		}
			
	}

	void OBJImporter::MeshInfo::Reset()
	{
		objectName.clear();
		groupName.clear();
		meshName.clear();
		materialName.clear();
		currentMesh._positions.clear();
		currentMesh._normals.clear();
		currentMesh._tangents.clear();
		currentMesh._texCoords.clear();
		currentMesh._vertexBones.clear();
		currentMesh._polyVerts.clear();
		materials.clear();
	}

}


