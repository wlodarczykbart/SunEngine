#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>

#include "ModelImporterMath.h"
#include "ModelImporterStr.h"

namespace ModelImporter
{
	class Importer
	{
	public:
		struct Options
		{
			Options();
			static Options Default();

			bool makeTwoSided;
		};

		struct Vertex
		{
			Vec3 position;
			Vec3 normal;
			Vec3 tangent;
			Vec2 texCoord;
		};

		struct VertexBoneInfo
		{
			static const uint32_t MAX_VERTEX_BONES = 8;

			float bones[MAX_VERTEX_BONES];
			float weights[MAX_VERTEX_BONES];
		};

		class FileStream
		{
		public:
			FileStream();
			~FileStream();

			bool OpenForWrite(const char* filename);
			bool OpenForRead(const char* filename);
			bool Close();

			void Write(const void* pData, uint32_t size);
			void Read(void* pData, const uint32_t size);

			void Write(const std::string& str);
			void Read(std::string& str);

			void Write(uint32_t value);
			void Read(uint32_t& value);

			void Write(float value);
			void Read(float& value);

			void Write(double value);
			void Read(double& value);

		private:
			FILE* _handle;
		};

		class ISerialize
		{
		public:
			ISerialize(uint32_t id);

			virtual void Write(FileStream& stream, Importer* pImporter);
			virtual void Read(FileStream& stream, Importer* pImporter);

			inline uint32_t GetID() const { return id; }

			static const uint32_t INVALID_ID = 0xFFFFFFF;

		private:	
			friend class Importer;
			uint32_t id;
		};

		class Texture : public ISerialize
		{
		private:
			friend class Importer;
			Texture(uint32_t id);
		public:
			//void Write(FileStream&  stream, Importer* pImporter) override;
			//void Read(FileStream&  stream, Importer* pImporter) override;

			std::string FileName;

		};

		class Material : public ISerialize
		{
		private:
			friend class Importer;
			Material(uint32_t id);
		public:
			//void Write(FileStream&  stream, Importer* pImporter) override;
			//void Read(FileStream&  stream, Importer* pImporter) override;

			std::string Name;

			Vec3 DiffuseColor;
			float Alpha;
			Vec3 SpecularColor;
			float SpecularExponent;

			Texture* DiffuseMap;
			Texture* NormalMap;
			Texture* SpecularMap;
			Texture* TransparentMap;

			void* pUserData;

		};

		enum NodeType
		{
			TRANSFORM,
			MESH,
			BONE,
		};

		class Node : public ISerialize
		{
		protected:
			friend class Importer;
			Node(NodeType type, uint32_t id);
		public:
			virtual ~Node() {}

			inline NodeType GetType() const { return Type; }
			void Attach(Node* pChild);
			void Detach();
			void CollectNodes(std::vector<Node*>& list);

			//virtual void Write(FileStream& stream, Importer* pImporter) override;
			//virtual void Read(FileStream& stream, Importer* pImporter) override;

			Node* Parent;
			std::list<Node*> Children;

			std::string Name;
			Mat4 LocalMatrix;
			Mat4 WorldMatrix;
			Mat4 GeometryOffset;
				 
		private:
			NodeType Type;
		};

		struct MeshData 
		{
			std::string Name;
			std::vector<Vertex> Vertices;
			std::vector<VertexBoneInfo> VertexBones;
			std::vector<uint32_t> Indices;
			AABB BoundingBox;
			uint32_t SkinIndex;
		};

		class Mesh : public Node
		{
		private:
			friend class Importer;
			Mesh(uint32_t id);
		public:
			//void Write(FileStream&  stream, Importer* pImporter) override;
			//void Read(FileStream&  stream, Importer* pImporter) override;

			MeshData* MeshData;
			Material* Material;

			void* pUserData;
		};

		class Bone : public Node
		{		
		private:
			friend class Importer;
			Bone(uint32_t id);
		public:
			//void Write(FileStream&  stream, Importer* pImporter) override;
			//void Read(FileStream&  stream, Importer* pImporter) override;

			inline Mat4 GetSkinnedMeshMatrix(Mesh * pMesh, Mat4 & boneTransform) const
			{
				Mat4 meshMtx = pMesh->WorldMatrix * pMesh->GeometryOffset;

				//Look at ViewScene project DrawScene.cxx line 493 for why this geometry offset is applied
				//for now its always identity it seems but if something weird is happening look into this
				Mat4 skinMtx = pMesh->GeometryOffset * SkinMatrices[pMesh->MeshData->SkinIndex];

				Mat4 meshInverse = boneTransform * meshMtx.GetInverse();
				Mat4 mtx = skinMtx * meshInverse;
				return mtx;
			}

			Vec3 Color;
			std::vector<Mat4> SkinMatrices;
			void* pUserData;
		};

		struct BoneTransform
		{
			Vec3 Translation;
			Vec3 Rotation;
			Vec3 Scale;
		};

		struct KeyFrame
		{
			double Time;
			std::vector<BoneTransform> Transforms;
		};

		class Animation : public ISerialize
		{
		private:
			friend class Importer;
			Animation(uint32_t id);
		public:
			//void Write(FileStream&  stream, Importer* pImporter) override;
			//void Read(FileStream&  stream, Importer* pImporter) override;

			std::string Name;
			double Length;
			std::vector<KeyFrame> KeyFrames;
			void* pUserData;

		};

		static Importer* Create(const std::string& filename);
		void Destroy();

		Importer();
		virtual ~Importer();
		
		bool Import(const Importer::Options& options = Importer::Options::Default());

		const std::string& GetFileName() const;

		const Options GetOptions() const;

		inline Mesh* GetMesh(uint32_t index) const { return Meshes.at(index); }
		inline Material* GetMaterial(uint32_t index)  const { return Materials.at(index); }
		inline Texture* GetTexture(uint32_t index) const { return Textures.at(index); }
		inline Animation* GetAnimation(uint32_t index) const { return Animations.at(index); }
		inline Bone* GetBone(uint32_t index) const { return Bones.at(index); }
		inline Node* GetNode(uint32_t index)  const { return Nodes.at(index); }
		inline MeshData* GetMeshData(uint32_t index) const { return MeshDatas.at(index); }

		inline uint32_t GetMeshCount() const { return Meshes.size(); }
		inline uint32_t GetMaterialCount()  const { return Materials.size(); }
		inline uint32_t GetTextureCount() const { return Textures.size(); }
		inline uint32_t GetAnimationCount() const { return Animations.size(); }
		inline uint32_t GetBoneCount() const { return Bones.size(); }
		inline uint32_t GetNodeCount()  const { return Nodes.size(); }
		inline uint32_t GetMeshDataCount() const { return MeshDatas.size(); }

		bool Save(const std::string& filename);
		bool Load(const std::string& filename);

	protected:
		struct PolygonVertex
		{
			int positionIndex;
			int normalIndex;
			int texCoordIndex;

			PolygonVertex()
			{
				positionIndex = -1;
				normalIndex = -1;
				texCoordIndex = -1;
			}
		};

		struct MeshInternalData
		{
			MeshInternalData() { _skinIndex = 0; }

			std::vector<Vec3> _positions;
			std::vector<Vec3> _normals;
			std::vector<Vec3> _tangents;
			std::vector<Vec2> _texCoords;
			std::vector<VertexBoneInfo> _vertexBones;
			std::vector<PolygonVertex> _polyVerts;
			uint32_t _skinIndex;
		};

		bool GetFullFilePath(const std::string& inPath, std::string& ouPath) const;
		Material* GetMaterial(const std::string& name);
		Material* GetMaterial(const void* pUserData);
		Texture* GetTexture(const std::string& filename);

		Mesh* AddMesh();
		Material* AddMaterial();
		Texture* AddTexture();
		Animation* AddAnimation();
		Bone* AddBone();
		Node* AddNode();

		Bone* FindBone(const void* pUserData) const;
		Bone* FindBone(const std::string& name) const;
		std::string GetUniqueNodeName(const std::string& base) const;

		virtual bool DerivedImport() { return true; }

		std::string _fileName;
		Options _options;

		private:
			void RemoveDuplicateIndices(MeshInternalData* pMesh);
			void ComputeNormals(MeshInternalData* pMesh);
			void ComputeTangents(MeshInternalData* pMesh);
			void MakeTwoSided(MeshData* pMesh);
			bool IsBonePathRemoveable(Node* pNode, uint32_t* pBoneUsageCheck) const;
			void RemoveNodes(std::list<Node*>& nodeRemoveList);

			template<typename T>
			void MakeNamesUnqiue(std::vector<T> &data)
			{
				for (uint32_t i = 0; i < data.size(); i++)
				{
					bool isDuplicate;
					do
					{
						isDuplicate = false;
						for (uint32_t j = 0; j < data.size(); j++)
						{
							if (i != j)
							{
								if (data[i]->Name == data[j]->Name)
								{
									data[i]->Name = data[i]->Name + "_1";
									isDuplicate = true;
									break;
								}
							}
						}

					} while (isDuplicate);
				}
			}

			std::vector<Mesh*> Meshes;
			std::vector<Material*> Materials;
			std::vector<Texture*> Textures;
			std::vector<Animation*> Animations;
			std::vector<Bone*> Bones;
			std::vector<Node*> Nodes;
			std::vector<MeshData*> MeshDatas;
	};

}