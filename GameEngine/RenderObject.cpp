#include "SceneNode.h"
#include "Material.h"
#include "RenderObject.h"

namespace SunEngine
{
	RenderObject::RenderObject()
	{

	}

	RenderObject::~RenderObject()
	{

	}

	RenderNode* RenderObject::CreateRenderNode(RenderComponentData* pData, Mesh* pMesh, Material* pMaterial, uint indexCount, uint instanceCount, uint firstIndex, uint vertexOffset)
	{
		pData->_renderNodes.push_back(RenderNode(this, pMesh, pMaterial, indexCount, instanceCount, firstIndex, vertexOffset));
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

	RenderNode::RenderNode(RenderObject* pObject, Mesh* pMesh, Material* pMaterial, uint idxCount, uint instanceCount, uint firstIdx, uint vtxOffset)
	{
		_renderObject = pObject;
		_mesh = pMesh;
		_material = pMaterial;
		_indexCount = idxCount;
		_instanceCount = instanceCount;
		_firstIndex = firstIdx;
		_vertexOffset = vtxOffset;

		_localAABB = 0;
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

		if (_material)
		{
			_material->BuildPipelineSettings(settings);
		}
	}
}