#pragma once

#include <unordered_map>
#include <list>

#include "3DImporter.h"

namespace ModelImporter
{
	class OBJImporter : public Importer
	{
	public:
		OBJImporter();
		~OBJImporter();

		bool DerivedImport() override;

	private:
		struct MaterialInfo
		{
			std::string name;
			uint32_t polyVertStart;
		};

		struct MeshInfo
		{
			MeshInfo();
			void Update();
			void Reset();

			std::string objectName;
			std::string groupName;
			std::string meshName;
			std::string materialName;
			MeshInternalData currentMesh;
			std::vector<MaterialInfo> materials;
		};

		MeshInternalData* GetCurrentMesh();
		Material* GetCurrentMaterial();

		typedef void(OBJImporter::*ProcessLine)(const std::string&, MeshInternalData*);
		typedef void(OBJImporter::*ProcessMatLine)(const std::string&, Material*);

		void ProcessVertexPosition(const std::string& line, MeshInternalData* pMesh);
		void ProcessVertexNormal(const std::string& line, MeshInternalData* pMesh);
		void ProcessVertexTexCoord(const std::string& line, MeshInternalData* pMesh);
		void ProcessFace(const std::string& line, MeshInternalData* pMesh);
		void ProcessMtlLib(const std::string& line, MeshInternalData* pMesh);
		void ProcessUseMtl(const std::string& line, MeshInternalData* pMesh);
		void ProcessGroupName(const std::string& line, MeshInternalData* pMesh);
		void ProcessObjectName(const std::string& line, MeshInternalData* pMesh);
		
		void ProcessNewMaterial(const std::string& line, Material* pMaterial);
		void ProcessDiffuseColor(const std::string& line, Material* pMaterial);
		void ProcessSpecularColor(const std::string& line, Material* pMaterial);
		void ProcessSpecularExponent(const std::string& line, Material* pMaterial);
		void ProcessDiffuseTexture(const std::string& line, Material* pMaterial);
		void ProcessNormalMapTexture(const std::string& line, Material* pMaterial);
		void ProcessDissolved(const std::string& line, Material* pMaterial);
		void ProcessTransparency(const std::string& line, Material* pMaterial);

		void FinishMesh();
		void ImportMaterialFile(const std::string& name);
		void RemapMesh(MeshInternalData* pMesh, std::vector<Vec3>& positions, std::vector<Vec3> &normals, std::vector<Vec2>& texCoords);

		std::string _currentType;
		std::string _previousType;

		std::unordered_map<std::string, ProcessLine> _processFuncs;
		std::unordered_map<std::string, ProcessMatLine> _processMatFuncs;

		MeshInfo _meshInfo;
		PolygonVertex _offsetVertex;
		
	};

}