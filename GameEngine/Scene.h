#pragma once

#include "SceneNode.h"
#include "BoundingVolumes.h"

namespace SunEngine
{
	class RenderNode;

	struct SceneRayHit
	{
		RenderNode* pHitNode;
		uint triIndex;
		float weights[3];
		glm::vec3 position;
		glm::vec3 normal;
	};

	class Scene
	{
	public:
		typedef void(*TraverseFunc)(SceneNode* pNode, void* pUserData);

		Scene();
		~Scene();

		SceneNode* GetRoot() const { return _root.get(); }
		const String& GetName() const { return _name; }

		SceneNode* AddNode(const String& name);
		SceneNode* GetNode(const String& name) const;
		bool RemoveNode(SceneNode* pNode);

		//void Initialize();
		void Update(float dt, float et);

		void Traverse(TraverseFunc func, void* pUserData) const;

		void Clear();

		void RegisterRenderNode(RenderNode* pNode);

		bool Raycast(const glm::vec3& o, const glm::vec3& d, SceneRayHit& hit) const;

	private:
		struct RenderNodeData
		{
			RenderNode* pNode;
			glm::mat4 mtx;
			glm::mat4 invMtx;
			AABB aabb;
			Sphere sphere;
		};

		//void CallInitialize(SceneNode* pNode);
		void CallUpdate(SceneNode* pNode, float dt, float et);
		void CallTraverse(SceneNode* pNode, TraverseFunc func, void* pUserData) const;
		void UpdateRenderNodes();

		friend class SceneMgr;

		String _name;
		UniquePtr<SceneNode> _root;
		StrMap<UniquePtr<SceneNode>> _nodes;
		Map<const RenderNode*, RenderNodeData> _renderNodes;
	};
}