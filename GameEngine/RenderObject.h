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

		const glm::mat4& GetWorld() const { return _worldMatrix; }
		const glm::mat4& GetInvWorldMatirx() const { return _invWorldMatrix; }
		const AABB& GetWorldAABB() const { return _worldAABB; }

	private:
		RenderNode(SceneNode *pNode, RenderObject* pObject);
		friend class RenderObject;

		glm::mat4 _worldMatrix;
		glm::mat4 _invWorldMatrix;

		SceneNode* _node;
		RenderObject* _renderObject;
		Mesh* _mesh;
		Material* _material;
		AABB _aabb;
		AABB _worldAABB;

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
		virtual void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

		virtual void Initialize(SceneNode* pNode, ComponentData* pData) override;
	protected:
		virtual RenderComponentData* AllocRenderData(SceneNode* pNode) = 0;
		virtual bool RequestData(RenderNode* pNode, RenderComponentData* pData, Mesh*& pMesh, Material*& pMaterial, const glm::mat4*& worldMtx, const AABB*& aabb, uint& idxCount, uint& instanceCount, uint& firstIdx, uint& vtxOffset) const = 0;

		RenderNode* CreateRenderNode(RenderComponentData* pData);

	private:
		void UpdateRenderNode(RenderNode& node, RenderComponentData* pData);
	};
}