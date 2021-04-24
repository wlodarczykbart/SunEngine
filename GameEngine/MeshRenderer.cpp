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
			MeshRendererComponentData* pRenderData = static_cast<MeshRendererComponentData*>(pData);
			pRenderData->_node = CreateRenderNode(pRenderData, _mesh, _material, _mesh->GetAABB(), _mesh->GetSphere(), _mesh->GetIndexCount(), 1, 0, 0);
		}

		RenderObject::Initialize(pSceneNode, pData);
	}

	void MeshRenderer::Update(SceneNode* pNode, ComponentData* pData, float, float)
	{
		MeshRendererComponentData* pRenderData = static_cast<MeshRendererComponentData*>(pData);
		pRenderData->_node->SetWorld(pNode->GetWorld());
	}

	void MeshRenderer::BuildPipelineSettings(PipelineSettings& settings) const
	{
		settings.inputAssembly.topology = SE_PT_TRIANGLE_LIST;
	}
}