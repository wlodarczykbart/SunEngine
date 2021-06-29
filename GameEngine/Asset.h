#pragma once

#include "Resource.h"
#include "AssetNode.h"

namespace SunEngine
{
	class SceneNode;
	class Scene;

	class Asset : public Resource
	{
	public:
		typedef void(*TraverseFunc)(AssetNode* pNode, void* pData);

		AssetNode* AddNode(const String& name);
		AssetNode* GetRoot() const { return _nodes.at(0).get(); }
		AssetNode* GetNodeByName(const String& name) const;

		bool SetParent(const String& child, const String& parent);

		SceneNode* CreateSceneNode(Scene* pScene, float assetScale = 0.0f) const;

		bool ComputeBoundingVolume(AABB& aabb) const;

		void Traverse(TraverseFunc func, void* pData) const;
	private:
		SceneNode* BuildSceneNode(AssetNode* pCurrNode, AssetNode* pCurrParent, Scene* pScene) const;

		Vector<UniquePtr<AssetNode>> _nodes;
	};
}