#pragma once

#include "AssetNode.h"
#include "PipelineSettings.h"
#include "BoundingVolumes.h"

#define DRAW_INDEXED(pRenderNode) pNode->GetIndexCount(), pNode->GetInstanceCount(), pNode->GetFirstIndex(), pNode->GetVertexOffset(), 0

namespace SunEngine
{
	class Material;
	class Mesh;
	class RenderObject;

	class RenderNode
	{
	public:
		~RenderNode();

		SceneNode* GetNode() const { return _node; }
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

		const AABB& GetAABB() const { return _aabb; }
		const Sphere& GetSphere() const { return _sphere; }

	private:
		RenderNode(SceneNode *pNode, RenderObject* pObject, Mesh* pMesh, Material* pMaterial, const AABB& aabb, const Sphere& sphere, uint idxCount, uint instanceCount, uint firstIdx, uint vtxOffset);
		friend class RenderObject;

		glm::mat4 _worldMatrix;

		SceneNode* _node;
		RenderObject* _renderObject;
		Mesh* _mesh;
		Material* _material;
		AABB _aabb;
		Sphere _sphere;

		uint _indexCount;
		uint _instanceCount;
		uint _firstIndex;
		uint _vertexOffset;
	};

	class RenderComponentData : public ComponentData
	{
	public:
		RenderComponentData(Component* pComponent, SceneNode* pNode) : ComponentData(pComponent, pNode) {}

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

		ComponentData* AllocData(SceneNode* pNode) override { return AllocRenderData(pNode); }

		virtual void Initialize(SceneNode* pNode, ComponentData* pData) override;

		bool CanRender() const override { return true; }

	protected:
		virtual RenderComponentData* AllocRenderData(SceneNode* pNode) { return new RenderComponentData(this, pNode); }

		RenderNode* CreateRenderNode(RenderComponentData* pData, Mesh* pMesh, Material* pMaterial, const AABB& aabb, const Sphere& sphere, uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset);
	};
}