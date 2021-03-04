#pragma once

#include "3DImporter.h"

namespace ModelImporter
{
	class FBXImporter : public Importer
	{
	public:
		static const char* FBX_DIFFUSE_MAP;
		static const char* FBX_NORMAL_MAP;
		static const char* FBX_SPECULAR_MAP;
		static const char* FBX_TRANSPARENT_MAP;
		static const char* FBX_AMBIENT_MAP;
		static const char* FBX_SPECUALR_FACTOR_MAP;
		static const char* FBX_SHININESS_EXPONENT_MAP;
		static const char* FBX_BUMP_MAP;


		FBXImporter();
		~FBXImporter();

		bool DerivedImport() override;


	private:
		class FbxNodeHandle;
		class FbxData;

		struct TraverseStack
		{
			FbxNodeHandle* pParent;
			Mat4 localMtx;
			Mat4 worldMtx;
			Mat4 geomOffset;
			void* pUserData;
		};

		typedef void (FBXImporter::*FbxTraverseFunc)(FbxNodeHandle* pHandle, TraverseStack& data);
		void Traverse(FbxTraverseFunc func);
		void Traverse(FbxTraverseFunc func, FbxNodeHandle* pHandle, TraverseStack data);
		void PrintNames(FbxNodeHandle* pHandle, TraverseStack& data);
		void CollectMeshes();
		void CollectMaterials();
		void UpdateAnimations();
		//void CollectMeshes(FbxNodeHandle* pHandle, TraverseStack& data);
		//void CollectSkeleton(FbxNodeHandle* pHandle, TraverseStack& data);
		void InitAnimationData();
		void InitNodes(FbxNodeHandle* pHandle, TraverseStack& data);
		void UpdateSkinningData();
		void CollectNodes(FbxNodeHandle* pHandle, TraverseStack& data);

		void CreateMesh(FbxNodeHandle* pHandle, std::vector<Node*>& output);

		FbxData* _fbxData;


		struct FbxMaterialMeshData
		{
			struct UniqueSet 
			{
				MeshInternalData* MeshData;
				Material* Material;
				std::vector<std::pair<void*, uint32_t>> MaterialPolyIndices;
			};

			MeshInternalData VertexData;
			void* Skin;
			std::vector<UniqueSet> Sets;
			std::vector<std::pair<uint32_t, uint32_t>> PolyIndexDataLookup;
		};

		std::unordered_map<void*, uint32_t> _skinIndexMap;
		std::unordered_map<Node*, void*> _nodeMap;
		std::unordered_map<void*, NodeType> _typeMap;
		std::vector<void*> _vertexBoneTable;
		std::unordered_map<void*, FbxMaterialMeshData> _existingMeshMap;
		//StrMap<String> _texNameToFilename;
	};

}