#pragma once

#include "SceneNode.h"

namespace SunEngine
{
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

	private:
		//void CallInitialize(SceneNode* pNode);
		void CallUpdate(SceneNode* pNode, float dt, float et);
		void CallTraverse(SceneNode* pNode, TraverseFunc func, void* pUserData) const;

		friend class SceneMgr;

		String _name;
		UniquePtr<SceneNode> _root;
		StrMap<UniquePtr<SceneNode>> _nodes;
	};
}