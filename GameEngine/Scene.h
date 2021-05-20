#pragma once

#include "SceneNode.h"
#include "SpatialVolumes.h"

namespace SunEngine
{
	class RenderNode;
	class CameraComponentData;
	class LightComponentData;
	class EnvironmentComponentData;

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
		typedef void(*TraverseRenderNodeFunc)(RenderNode* pNode, void* pUserData);
		typedef bool(*TraverseAABBFunc)(const AABB& box, void* pUserData);

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

		void RegisterLight(LightComponentData* pLight);
		const LinkedList<LightComponentData*>& GetLightList() const { return _lightList; }

		void RegisterCamera(CameraComponentData* pCamera);
		const LinkedList<CameraComponentData*>& GetCameraList() const { return _cameraList; }

		void RegisterEnvironment(EnvironmentComponentData* pEnvironment);
		const LinkedList<EnvironmentComponentData*>& GetEnvironmentList() const { return _environmentList; }

		void TraverseRenderNodes(TraverseAABBFunc aabbFunc, void* pAABBData, TraverseRenderNodeFunc nodeFunc, void* pNodeData);

		bool Raycast(const glm::vec3& o, const glm::vec3& d, SceneRayHit& hit) const;
	private:
		//class SceneGrid;

		//void CallInitialize(SceneNode* pNode);
		void CallUpdate(SceneNode* pNode, float dt, float et);
		void CallTraverse(SceneNode* pNode, TraverseFunc func, void* pUserData) const;
		void RebuildBoxTree();

		friend class SceneMgr;

		String _name;
		UniquePtr<SceneNode> _root;
		StrMap<UniquePtr<SceneNode>> _nodes;
		//UniquePtr<SceneGrid> _grid;

		QuadTree<RenderNode*> _boxTree;
		LinkedList<RenderNode*> _pendingBoxTreeNodes;

		struct BoxTreeSizeData;
		UniquePtr<BoxTreeSizeData> _boxTreeSizeData;

		LinkedList<LightComponentData*> _lightList;
		LinkedList<CameraComponentData*> _cameraList;
		LinkedList<EnvironmentComponentData*> _environmentList;
	};
}