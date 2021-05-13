#include "Mesh.h"
#include "Material.h"
#include "SceneNode.h"

#include "MeshRenderer.h"

namespace SunEngine
{
	const ComponentType MeshRenderer::CType = COMPONENT_MESH_RENDERER;

	MeshRenderer::MeshRenderer()
	{
		_mesh = 0;
		_material = 0;
	}

	MeshRenderer::~MeshRenderer()
	{
	}

	void MeshRenderer::Initialize(SceneNode* pSceneNode, ComponentData* pData)
	{
		if (_mesh)
		{
			MeshRendererComponentData* pRenderData = pData->As<MeshRendererComponentData>();
			pRenderData->_node = CreateRenderNode(pRenderData);
		}

		RenderObject::Initialize(pSceneNode, pData);
	}

	void MeshRenderer::Update(SceneNode* pNode, ComponentData* pData, float dt, float et)
	{
		RenderObject::Update(pNode, pData, dt, et);
	}

	void MeshRenderer::BuildPipelineSettings(PipelineSettings& settings) const
	{
		settings.inputAssembly.topology = SE_PT_TRIANGLE_LIST;
	}

	bool MeshRenderer::RequestData(RenderNode* pNode, RenderComponentData* pData, Mesh*& pMesh, Material*& pMaterial, const glm::mat4*& worldMtx, const AABB*& aabb, uint& idxCount, uint& instanceCount, uint& firstIdx, uint& vtxOffset) const
	{
		MeshRendererComponentData* pRenderData = pData->As<MeshRendererComponentData>();
		assert(pRenderData->_node == pNode);
		if (pRenderData->_node != pNode)
			return false;

		pMesh = _mesh;
		pMaterial = _material;
		worldMtx = &pNode->GetNode()->GetWorld();
		aabb = &pMesh->GetAABB();
		idxCount = pMesh->GetIndexCount();
		instanceCount = 1;
		firstIdx = 0;
		vtxOffset = 0;
		return true;
	}
}