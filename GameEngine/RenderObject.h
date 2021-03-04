#pragma once

#include "AssetNode.h"
#include "PipelineSettings.h"

#define DRAW_INDEXED(pRenderNode) pNode->GetIndexCount(), pNode->GetInstanceCount(), pNode->GetFirstIndex(), pNode->GetVertexOffset(), 0

namespace SunEngine
{
	class Material;
	class Mesh;
	class RenderObject;
	struct AABB;

	class RenderNode
	{
	public:
		~RenderNode();

		RenderObject* GetRenderObject() const { return _renderObject; }
		Mesh* GetMesh() const { return _mesh; }
		Material* GetMaterial() const { return _material; }

		uint GetIndexCount() const { return _indexCount; }
		uint GetInstanceCount() const { return _instanceCount; }
		uint GetFirstIndex() const { return _firstIndex; }
		uint GetVertexOffset() const { return _vertexOffset; }

		void BuildPipelineSettings(PipelineSettings& settings) const;

		void SetWorld(const glm::mat4& mtx) { _worldMatrix = mtx; }
		const glm::mat4& GetWorld() const { return _worldMatrix; }

		void SetLocalAABB(const AABB* localAABB) { _localAABB = localAABB; }
		const AABB* GetLocalAABB() const { return _localAABB; }

	private:
		RenderNode(RenderObject* pObject, Mesh* pMesh, Material* pMaterial, uint idxCount, uint instanceCount, uint firstIdx, uint vtxOffset);
		friend class RenderObject;

		glm::mat4 _worldMatrix;

		RenderObject* _renderObject;
		Mesh* _mesh;
		Material* _material;
		const AABB* _localAABB;

		uint _indexCount;
		uint _instanceCount;
		uint _firstIndex;
		uint _vertexOffset;
	};

	class RenderComponentData : public ComponentData
	{
	public:
		RenderComponentData(Component* pComponent) : ComponentData(pComponent) {}

		LinkedList<RenderNode>::const_iterator BeginNode() const { return _renderNodes.begin(); }
		LinkedList<RenderNode>::const_iterator EndNode() const { return _renderNodes.end(); }

	private:
		friend class RenderObject;
		LinkedList<RenderNode> _renderNodes;
	};

	class RenderObject : public Component
	{
	public:
		RenderObject();
		virtual ~RenderObject();

		virtual void BuildPipelineSettings(PipelineSettings&) const {};

		ComponentData* AllocData() override { return AllocRenderData(); }

		bool CanRender() const override { return true; }

	protected:
		virtual RenderComponentData* AllocRenderData() { return new RenderComponentData(this); }

		RenderNode* CreateRenderNode(RenderComponentData* pData, Mesh* pMesh, Material* pMaterial, uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset);
	};
}