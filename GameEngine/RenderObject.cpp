#include "SceneNode.h"
#include "Material.h"
#include "Scene.h"
#include "RenderObject.h"

namespace SunEngine
{
	RenderObject::RenderObject()
	{
		
	}

	RenderObject::~RenderObject()
	{

	}

	void RenderObject::Update(SceneNode* pNode, ComponentData* pData, float dt, float et)
	{
		RenderComponentData* pRenderData = pData->As<RenderComponentData>();
		for (RenderNode& node : pRenderData->_renderNodes)
		{
			UpdateRenderNode(node, pRenderData);
		}
	}

	void RenderObject::Initialize(SceneNode* pNode, ComponentData* pData)
	{
		Scene* pScene = pNode->GetScene();
		RenderComponentData* pRenderData = pData->As<RenderComponentData>();

		for (RenderNode& node : pRenderData->_renderNodes)
		{
			UpdateRenderNode(node, pRenderData);
			pScene->RegisterRenderNode(&node);
		}
	}

	RenderNode* RenderObject::CreateRenderNode(RenderComponentData* pData)
	{
		pData->_renderNodes.push_back(RenderNode(pData->GetNode(), this));
		return &pData->_renderNodes.back();
	}

	void RenderObject::UpdateRenderNode(RenderNode& node, RenderComponentData* pRenderData)
	{
		const glm::mat4* pMtx = NULL;
		const AABB* pAABB = NULL;
		RequestData(&node, pRenderData, node._mesh, node._material, pMtx, pAABB, node._indexCount, node._instanceCount, node._firstIndex, node._vertexOffset);

		bool matrixChanged = node._worldMatrix != *pMtx;
		bool aabbChanged = node._aabb != *pAABB;

		if (matrixChanged)
		{
			node._worldMatrix = *pMtx;
			node._invWorldMatrix = glm::inverse(node._worldMatrix);
		}

		if(aabbChanged)
			node._aabb = *pAABB;

		if (matrixChanged || aabbChanged)
		{
			node._worldAABB = node._aabb;
			node._worldAABB.Transform(node._worldMatrix);
		}
	}

	RenderNode::RenderNode(SceneNode* pNode, RenderObject* pObject)
	{
		_node = pNode;
		_renderObject = pObject;
		_mesh = 0;
		_material = 0;
		_indexCount = 0;
		_instanceCount = 0;
		_firstIndex = 0;
		_vertexOffset = 0;
		_aabb.Reset();
		_worldAABB.Reset();
		_worldMatrix = glm::mat4(1.0f);
		_invWorldMatrix = glm::mat4(1.0f);
	}

	RenderNode::~RenderNode()
	{

	}

	void RenderNode::BuildPipelineSettings(PipelineSettings& settings) const
	{
		if (_renderObject)
		{
			_renderObject->BuildPipelineSettings(settings);
		}
	}
}