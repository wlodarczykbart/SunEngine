#pragma once

#include "RenderObject.h"

namespace SunEngine
{
	class TerrainComponentData : public RenderComponentData
	{
	public:
		TerrainComponentData(Component* pComponent, SceneNode* pNode) : RenderComponentData(pComponent, pNode) { }

	private:
		friend class Terrain;
		Map<RenderNode*, uint> _sliceNodes;
	};

	class Terrain : public RenderObject
	{
	public:
		Terrain();
		~Terrain();

		void SetResolution(uint resolution) { _resolution = resolution; }
		uint GetResolution() const { return _resolution; }

		void SetSlices(uint slices) { _slices = slices; }
		uint GetSlices() const { return _slices; }

		void BuildMesh();

		void Initialize(SceneNode* pNode, ComponentData* pData) override;
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

		Material* GetMaterial() const { return _material.get(); }
		Mesh* GetMesh() const { return _mesh.get(); }

	private:
		struct Slice
		{
			uint x;
			uint y;
			uint firstIndex;
			uint indexCount;
			uint vertexOffset;
			AABB aabb;
		};

		RenderComponentData* AllocRenderData(SceneNode* pNode) { return new TerrainComponentData(this, pNode); }
		bool RequestData(RenderNode* pNode, RenderComponentData* pData, Mesh*& pMesh, Material*& pMaterial, const glm::mat4*& worldMtx, const AABB*& aabb, uint& idxCount, uint& instanceCount, uint& firstIdx, uint& vtxOffset) const override;
		void BuildSliceIndices(Map<glm::uvec2, Vector<uint>>& sliceTypeIndices, uint& indexCount) const;
		void BuildPipelineSettings(PipelineSettings& settings) const override;

		uint _resolution;
		uint _slices;
		Vector<Slice> _sliceData;

		UniquePtr<Mesh> _mesh;
		UniquePtr<Material> _material;
	};
}