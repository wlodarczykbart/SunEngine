#pragma once

#include "Resource.h"
#include "AssetNode.h"
#include "BoundingVolumes.h"

namespace SunEngine
{
	class SceneNode;
	class Scene;

	class Asset : public Resource
	{
	public:

		AssetNode* AddNode(const String& name);
		AssetNode* GetRoot() const { return _nodes.at(0).get(); }
		AssetNode* GetNodeByName(const String& name) const;

		bool SetParent(const String& child, const String& parent);

		SceneNode* CreateSceneNode(Scene* pScene) const;

		void UpdateBoundingVolume();

		const AABB& GetAABB() const { return _aabb; }
	private:
		SceneNode* BuildSceneNode(AssetNode* pCurrNode, AssetNode* pCurrParent, Scene* pScene) const;

		Vector<UniquePtr<AssetNode>> _nodes;
		AABB _aabb;
	};
}