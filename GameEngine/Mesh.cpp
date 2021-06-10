#include "Mesh.h"

#define SWIZZLE_YZX(v) glm::vec4(v.y, v.z, v.z, v.w)

namespace SunEngine
{
	const uint VertexDef::DEFAULT_INVALID_INDEX = (uint)-1;
	const uint VertexDef::DEFAULT_TEX_COORD_INDEX = 1;
	const uint VertexDef::DEFAULT_NORMAL_INDEX = 2;
	const uint VertexDef::DEFAULT_TANGENT_INDEX = 3;

	const VertexDef StandardVertex::Definition = VertexDef(4, VertexDef::DEFAULT_TEX_COORD_INDEX, VertexDef::DEFAULT_NORMAL_INDEX, VertexDef::DEFAULT_TANGENT_INDEX);
	const VertexDef TerrainVertex::Definition = VertexDef(2, VertexDef::DEFAULT_INVALID_INDEX, 1, VertexDef::DEFAULT_INVALID_INDEX);

	Mesh::Mesh()
	{
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::AllocVertices(uint numVerts, const VertexDef& def)
	{
		_vertexDef = def;
		_vertices.resize(numVerts * def.NumVars);
	}

	void Mesh::AllocIndices(uint numIndices)
	{
		_indices.resize(numIndices);
	}

	bool Mesh::RegisterToGPU()
	{
		if (!_gpuObject.Destroy())
			return false;

		BaseMesh::CreateInfo info = {};
		info.numIndices = GetIndexCount();
		info.numVerts = GetVertexCount();
		info.pVerts = _vertices.data();
		info.pIndices = _indices.data();
		info.vertexStride = GetVertexStride();

		if (!_gpuObject.Create(info))
			return false;

		return true;
	}

	void Mesh::AllocateCube()
	{
		AllocVertices(24, StandardVertex::Definition );
		AllocIndices(36);

		uint vtx = 0;

		//Back
		SetVertexVar(vtx, glm::vec4(-0.5f, -0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, -0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.5f, +0.5f, 1.0f)); vtx++;

		//Front
		SetVertexVar(vtx, glm::vec4(+0.5f, -0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.5f, -0.5f, 1.0f)); vtx++;

		//Left
		SetVertexVar(vtx, glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, -0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.5f, -0.5f, 1.0f)); vtx++;

		//Right
		SetVertexVar(vtx, glm::vec4(+0.5f, -0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, -0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.5f, +0.5f, 1.0f)); vtx++;

		//Top
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.5f, -0.5f, 1.0f)); vtx++;

		//Bottom
		SetVertexVar(vtx, glm::vec4(-0.5f, -0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, -0.5f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, -0.5f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, -0.5f, +0.5f, 1.0f)); vtx++;

		glm::vec4 tcQuad[] = { glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), };

		uint idx = 0;
		for (uint i = 0; i < GetVertexCount(); i += 4)
		{
			glm::vec4 normal = glm::vec4(glm::normalize(glm::cross(glm::vec3(GetVertexPos(i + 1) - GetVertexPos(i + 0)), glm::vec3(GetVertexPos(i + 2) - GetVertexPos(i + 0)))), 0.0f);
			glm::vec4 tangent = glm::vec4(-normal.y, normal.z, normal.x, 0.0f);

			for (uint j = 0; j < 4; j++)
			{
				SetVertexVar(i + j, tcQuad[j], VertexDef::DEFAULT_TEX_COORD_INDEX);
				SetVertexVar(i + j, normal, VertexDef::DEFAULT_NORMAL_INDEX);
				SetVertexVar(i + j, tangent, VertexDef::DEFAULT_TANGENT_INDEX);
			}

			_indices[idx++] = i + 0;
			_indices[idx++] = i + 1;
			_indices[idx++] = i + 2;
			_indices[idx++] = i + 0;
			_indices[idx++] = i + 2;
			_indices[idx++] = i + 3;
		}

		UpdateBoundingVolume();
	}

	void Mesh::AllocateSphere()
	{
		Vector<StandardVertex> vertices;

		float radius = 1.0f;
		uint stackCount = 32;
		uint sliceCount = 32;
	   
		//
		// Compute the vertices stating at the top pole and moving down the stacks.
		//

		// Poles: note that there will be texture coordinate distortion as there is
		// not a unique point on the texture map to assign to the pole when mapping
		// a rectangular texture onto a sphere.
		StandardVertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		StandardVertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

		vertices.push_back(topVertex);


		float phiStep = glm::pi<float>() / stackCount;
		float thetaStep = 2.0f * glm::pi<float>() / sliceCount;

		// Compute vertices for each stack ring (do not count the poles as rings).
		for (uint i = 1; i <= stackCount - 1; ++i)
		{
			float phi = i * phiStep;

			// Vertices of ring.
			for (uint j = 0; j <= sliceCount; ++j)
			{
				float theta = j * thetaStep;

				StandardVertex v;

				// spherical to cartesian
				v.Position.x = radius * sinf(phi) * cosf(theta);
				v.Position.y = radius * cosf(phi);
				v.Position.z = radius * sinf(phi) * sinf(theta);

				// Partial derivative of P with respect to theta
				v.Tangent.x = -radius * sinf(phi) * sinf(theta);
				v.Tangent.y = 0.0f;
				v.Tangent.z = +radius * sinf(phi) * cosf(theta);
				v.Tangent = glm::normalize(v.Tangent);

				v.Normal = glm::normalize(glm::vec4(glm::vec3(v.Position), 0.0f));

				v.TexCoord.x = theta / glm::two_pi<float>();
				v.TexCoord.y = phi / glm::pi<float>();

				vertices.push_back(v);
			}
		}

		vertices.push_back(bottomVertex);

		//
		// Compute indices for top stack.  The top stack was written first to the vertex buffer
		// and connects the top pole to the first ring.
		//

		_indices.clear();

		for (uint i = 1; i <= sliceCount; ++i)
		{
			_indices.push_back(0);
			_indices.push_back(i + 1);
			_indices.push_back(i);
		}

		//
		// Compute indices for inner stacks (not connected to poles).
		//

		// Offset the indices to the index of the first vertex in the first ring.
		// This is just skipping the top pole vertex.
		uint baseIndex = 1;
		uint ringVertexCount = sliceCount + 1;
		for (uint i = 0; i < stackCount - 2; ++i)
		{
			for (uint j = 0; j < sliceCount; ++j)
			{
				_indices.push_back(baseIndex + i * ringVertexCount + j);
				_indices.push_back(baseIndex + i * ringVertexCount + j + 1);
				_indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

				_indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
				_indices.push_back(baseIndex + i * ringVertexCount + j + 1);
				_indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
			}
		}

		//
		// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
		// and connects the bottom pole to the bottom ring.
		//

		// South pole vertex was added last.
		uint southPoleIndex = (uint)vertices.size() - 1;

		// Offset the indices to the index of the first vertex in the last ring.
		baseIndex = southPoleIndex - ringVertexCount;

		for (uint i = 0; i < sliceCount; ++i)
		{
			_indices.push_back(southPoleIndex);
			_indices.push_back(baseIndex + i);
			_indices.push_back(baseIndex + i + 1);
		}

		AllocVertices(vertices.size(), StandardVertex::Definition);
		for (uint i = 0; i < vertices.size(); i++)
		{
			SetVertexVar(i, vertices[i].Position, 0);
			SetVertexVar(i, vertices[i].TexCoord, _vertexDef.TexCoordIndex);
			SetVertexVar(i, vertices[i].Normal, _vertexDef.NormalIndex);
			SetVertexVar(i, vertices[i].Tangent, _vertexDef.TangentIndex);
		}

		UpdateBoundingVolume();
	}

	void Mesh::AllocatePlane()
	{
		AllocVertices(8, StandardVertex::Definition);
		AllocIndices(12);

		uint vtx = 0;

		//Top
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.0f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.0f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.0f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.0f, -0.5f, 1.0f)); vtx++;

		//Bottom
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.0f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.0f, -0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.0f, +0.5f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.0f, +0.5f, 1.0f)); vtx++;

		glm::vec4 tcQuad[] = { glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), };

		uint idx = 0;
		for (uint i = 0; i < GetVertexCount(); i += 4)
		{
			glm::vec4 normal = glm::vec4(glm::normalize(glm::cross(glm::vec3(GetVertexPos(i + 1) - GetVertexPos(i + 0)), glm::vec3(GetVertexPos(i + 2) - GetVertexPos(i + 0)))), 0.0f);
			glm::vec4 tangent = glm::vec4(-normal.y, normal.z, normal.x, 0.0f);

			for (uint j = 0; j < 4; j++)
			{
				SetVertexVar(i + j, tcQuad[j], VertexDef::DEFAULT_TEX_COORD_INDEX);
				SetVertexVar(i + j, normal, VertexDef::DEFAULT_NORMAL_INDEX);
				SetVertexVar(i + j, tangent, VertexDef::DEFAULT_TANGENT_INDEX);
			}

			_indices[idx++] = i + 0;
			_indices[idx++] = i + 1;
			_indices[idx++] = i + 2;
			_indices[idx++] = i + 0;
			_indices[idx++] = i + 2;
			_indices[idx++] = i + 3;
		}

		UpdateBoundingVolume();
	}

	void Mesh::AllocateQuad()
	{
		AllocVertices(4, StandardVertex::Definition);
		AllocIndices(6);

		uint vtx = 0;

		//Top
		SetVertexVar(vtx, glm::vec4(-0.5f, -0.5f, +0.0f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, -0.5f, +0.0f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(+0.5f, +0.5f, -0.0f, 1.0f)); vtx++;
		SetVertexVar(vtx, glm::vec4(-0.5f, +0.5f, -0.0f, 1.0f)); vtx++;

		glm::vec4 tcQuad[] = { glm::vec4(0.0f, 1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), };

		uint idx = 0;
		for (uint i = 0; i < GetVertexCount(); i += 4)
		{
			glm::vec4 normal = glm::vec4(glm::normalize(glm::cross(glm::vec3(GetVertexPos(i + 1) - GetVertexPos(i + 0)), glm::vec3(GetVertexPos(i + 2) - GetVertexPos(i + 0)))), 0.0f);
			glm::vec4 tangent = glm::vec4(-normal.y, normal.z, normal.x, 0.0f);

			for (uint j = 0; j < 4; j++)
			{
				SetVertexVar(i + j, tcQuad[j], VertexDef::DEFAULT_TEX_COORD_INDEX);
				SetVertexVar(i + j, normal, VertexDef::DEFAULT_NORMAL_INDEX);
				SetVertexVar(i + j, tangent, VertexDef::DEFAULT_TANGENT_INDEX);
			}

			_indices[idx++] = i + 0;
			_indices[idx++] = i + 2;
			_indices[idx++] = i + 1;
			_indices[idx++] = i + 0;
			_indices[idx++] = i + 3;
			_indices[idx++] = i + 2;
		}

		UpdateBoundingVolume();
	}

	void Mesh::SetVertexVar(uint vertexIndex, const glm::vec4& value, uint varIndex)
	{
		if (varIndex < _vertexDef.NumVars)
		{
			_vertices[vertexIndex * _vertexDef.NumVars + varIndex] = value;
		}
	}

	void Mesh::SetTri(uint triIndex, uint t0, uint t1, uint t2)
	{
		_indices[triIndex * 3 + 0] = t0;
		_indices[triIndex * 3 + 1] = t1;
		_indices[triIndex * 3 + 2] = t2;
	}

	void Mesh::SetIndices(const uint* pIndices, uint indexOffset, uint indexCount)
	{
		memcpy(_indices.data() + indexOffset, pIndices, indexCount * sizeof(uint));
	}

	void Mesh::GetTri(uint triIndex, uint& t0, uint& t1, uint& t2) const
	{
		t0 = _indices[triIndex * 3 + 0];
		t1 = _indices[triIndex * 3 + 1];
		t2 = _indices[triIndex * 3 + 2];
	}

	glm::vec4 Mesh::GetVertexVar(uint vertexIndex, uint varIndex) const
	{
		if (varIndex < _vertexDef.NumVars)
		{
			return _vertices.at(vertexIndex * _vertexDef.NumVars + varIndex);
		}
		else
		{
			return glm::vec4(0, 0, 0, 0);
		}
	}

	glm::vec4 Mesh::GetVertexPos(uint vertexIndex) const
	{
		return GetVertexVar(vertexIndex, 0);
	}

	void Mesh::UpdateBoundingVolume()
	{
		_aabb.Reset();
		for (uint i = 0; i < GetVertexCount(); i++)
		{
			_aabb.Expand(GetVertexVar(i, 0));
		}

		_sphere.Center = _aabb.GetCenter();
		float maxDist = -FLT_MAX;
		for (uint i = 0; i < GetVertexCount(); i++)
		{
			glm::vec3 v =_sphere.Center - glm::vec3(GetVertexVar(i, 0));
			float dist = glm::dot(v, v);
			maxDist = glm::max(maxDist, dist);
		}
		_sphere.Radius = sqrtf(maxDist);
	}

	bool Mesh::UpdateVertices()
	{
		if(!_gpuObject.UpdateVertices(_vertices.data(), GetVertexStride() * GetVertexCount()))
			return false;

		UpdateBoundingVolume();
		return true;
	}
}