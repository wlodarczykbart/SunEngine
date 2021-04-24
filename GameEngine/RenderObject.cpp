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

	void RenderObject::Initialize(SceneNode* pNode, ComponentData* pData)
	{
		Scene* pScene = pNode->GetScene();
		RenderComponentData* pRenderData = static_cast<RenderComponentData*>(pData);

		for (auto& node : pRenderData->_renderNodes)
		{
			pScene->RegisterRenderNode(&node);
		}
	}

	RenderNode* RenderObject::CreateRenderNode(RenderComponentData* pData, Mesh* pMesh, Material* pMaterial, const AABB& aabb, const Sphere& sphere, uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset)
	{
		pData->_renderNodes.push_back(RenderNode(pData->GetNode(), this, pMesh, pMaterial, aabb, sphere, indexCount, instanceCount, firstIndex, vertexOffset));
		return &pData->_renderNodes.back();
	}

	//RenderNode::RenderNode()
	//{
	//	_renderObject = 0;
	//	_mesh = 0;
	//	_material = 0;
	//	_indexCount = 0;
	//	_instanceCount = 0;
	//	_vertexOffset = 0;
	//	_firstIndex = 0;
	//}

	RenderNode::RenderNode(SceneNode* pNode, RenderObject* pObject, Mesh* pMesh, Material* pMaterial, const AABB& aabb, const Sphere& sphere, uint idxCount, uint instanceCount, uint firstIdx, uint vtxOffset)
	{
		_node = pNode;
		_renderObject = pObject;
		_mesh = pMesh;
		_material = pMaterial;
		_indexCount = idxCount;
		_instanceCount = instanceCount;
		_firstIndex = firstIdx;
		_vertexOffset = vtxOffset;
		_aabb = aabb;
		_sphere = sphere;
		_worldMatrix = glm::mat4(1.0);
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