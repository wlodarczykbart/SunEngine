#pragma once

#include "RenderObject.h"

namespace SunEngine
{
	class MeshRendererComponentData : public RenderComponentData
	{
	public:
		MeshRendererComponentData(Component* pComponent, SceneNode* pNode) : RenderComponentData(pComponent, pNode) { _node = 0; }

	private:
		friend class MeshRenderer;

		RenderNode* _node;
	};

	class MeshRenderer : public RenderObject
	{
	public:
		static const ComponentType CType;

		MeshRenderer();
		~MeshRenderer();
		ComponentType GetType() const override { return CType; }

		void SetMesh(Mesh* pMesh) { _mesh = pMesh; }
		void SetMaterial(Material* pMaterial) { _material = pMaterial; }

		Mesh* GetMesh() const { return _mesh; }
		Material* GetMaterial() const { return _material; }

		void Initialize(SceneNode* pNode, ComponentData* pData) override;
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

		void BuildPipelineSettings(PipelineSettings& settings) const override;
		
	protected:
		RenderComponentData* AllocRenderData(SceneNode* pNode) { return new MeshRendererComponentData(this, pNode); }

	private:
		Mesh* _mesh;
		Material* _material;
	};
}