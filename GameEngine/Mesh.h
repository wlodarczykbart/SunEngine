#pragma once

#include "BaseMesh.h"
#include "GPUResource.h"

namespace SunEngine
{
	struct VertexDef
	{
		VertexDef() { NumVars = 1; TexCoordIndex = DEFAULT_INVALID_INDEX; NormalIndex = DEFAULT_INVALID_INDEX; TangentIndex = DEFAULT_INVALID_INDEX; }
		VertexDef(uint numVars, uint texCoordIndex, uint normalIndex, uint tangentIndex) { NumVars = numVars; TexCoordIndex = texCoordIndex; NormalIndex = normalIndex; TangentIndex = tangentIndex; }

		uint NumVars;
		uint TexCoordIndex;
		uint NormalIndex;
		uint TangentIndex;

		static const uint DEFAULT_INVALID_INDEX;
		static const uint DEFAULT_TEX_COORD_INDEX;
		static const uint DEFAULT_NORMAL_INDEX;
		static const uint DEFAULT_TANGENT_INDEX;
	};

	struct StandardVertex
	{
		static const VertexDef Definition;

		StandardVertex()
		{
			Position = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			Normal = Vec4::Zero;
			Tangent = Vec4::Zero;
			TexCoord = Vec4::Zero;
		}

		StandardVertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v)
		{
			Position = glm::vec4(px, py, pz, 1.0f);
			Normal = glm::vec4(nx, ny, nz, 0.0f);
			Tangent = glm::vec4(tx, ty, tz, 0.0f);
			TexCoord = glm::vec4(u, v, 0.0f, 0.0f);
		}

		glm::vec4 Position;
		glm::vec4 TexCoord;
		glm::vec4 Normal;
		glm::vec4 Tangent;
	};

	struct SkinnedVertex
	{
		static const VertexDef Definition;

		StandardVertex Standard;
		glm::vec4 Bones;
		glm::vec4 Weights;
	};

	struct TerrainVertex
	{
		static const VertexDef Definition;

		TerrainVertex()
		{
			Position = Vec4::Point;
			Normal = Vec4::Up;
		}

		glm::vec4 Position;
		glm::vec4 Normal;
	};

	class Mesh : public GPUResource<BaseMesh>
	{
	public:
		Mesh();
		~Mesh();

		bool RegisterToGPU() override;

		void AllocVertices(uint numVerts, const VertexDef& def);
		void AllocIndices(uint numIndices);

		uint GetVertexCount() const { return _vertices.size() / _vertexDef.NumVars; }
		uint GetIndexCount() const { return _indices.size(); }
		uint GetTriCount() const { return _indices.size() / 3; }
		uint GetVertexStride() const { return sizeof(glm::vec4) * _vertexDef.NumVars; }

		void SetVertexVar(uint vertexIndex, const glm::vec4& value, uint varIndex = 0);
		void SetTri(uint triIndex, uint t0, uint t1, uint t2);
		void GetTri(uint triIndex, uint& t0, uint& t1, uint& t2) const;
		
		template<typename T>
		T* GetVertices()
		{
			if (GetVertexStride() == sizeof(T))
				return reinterpret_cast<T*>(_vertices.data());
			else
				return 0;
		}

		void SetIndices(const uint* pIndices, uint indexOffset, uint indexCount);
		uint GetIndex(uint index) const { return _indices.at(index); }

		glm::vec4 GetVertexVar(uint vertexIndex, uint varIndex) const;
		glm::vec4 GetVertexPos(uint vertexIndex) const;

		const VertexDef& GetVertexDef() const { return _vertexDef; }

		void AllocateCube();
		void AllocateSphere();
		void AllocatePlane();
		void AllocateQuad();

		const AABB& GetAABB() const { return _aabb; }
		const Sphere& GetSphere() const { return _sphere; }
		void UpdateBoundingVolume();

		bool UpdateVertices();

	private:
		VertexDef _vertexDef;
		Vector<glm::vec4> _vertices;
		Vector<uint> _indices;
		AABB _aabb;
		Sphere _sphere;
	};
}