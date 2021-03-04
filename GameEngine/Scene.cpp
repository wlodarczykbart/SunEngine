#include "StringUtil.h"
#include "Scene.h"

#define SCENE_ROOT_NAME "SceneRoot"

namespace SunEngine
{
	Scene::Scene()
	{
		_root = UniquePtr<SceneNode>(new SceneNode());
		_root->_name = SCENE_ROOT_NAME;
	}

	Scene::~Scene()
	{
	}

	SceneNode* Scene::AddNode(const String& name)
	{
		uint counter = 0;
		String uniqueName = name;
		while (_nodes.find(uniqueName) != _nodes.end())
		{
			uniqueName = StrFormat("%s_%d", name.c_str(), ++counter);
		}

		SceneNode* pNode = new SceneNode();
		pNode->_name = uniqueName;
		_nodes[uniqueName] = UniquePtr<SceneNode>(pNode);

		pNode->SetParent(GetRoot());
		return pNode;
	}

	SceneNode* Scene::GetNode(const String& name) const
	{
		auto found = _nodes.find(name);
		return found != _nodes.end() ? (*found).second.get() : 0;
	}

	bool Scene::RemoveNode(SceneNode* pNode)
	{
		if (!pNode)
			return false;

		if (pNode == GetRoot())
			return false;

		auto found = _nodes.find(pNode->GetName());
		if (found != _nodes.end() && (*found).second.get() == pNode)
		{
			pNode->SetParent(0);
			LinkedList<SceneNode*> nodesToRemove;
			pNode->Traverse([](SceneNode* pRemoveNode, void* pNodeMap) -> bool {
				static_cast<LinkedList<SceneNode*>*>(pNodeMap)->push_front(pRemoveNode);
				return true;
			}, & nodesToRemove);

			for (auto iter = nodesToRemove.begin(); iter != nodesToRemove.end(); ++iter)
			{
				_nodes.erase((*iter)->GetName());
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	//void Scene::Initialize()
	//{
	//	CallInitialize(GetRoot());
	//}

	//void Scene::CallInitialize(SceneNode* pNode)
	//{
	//	pNode->Initialize();
	//	for (auto iter = pNode->_children.begin(); iter != pNode->_children.end(); ++iter)
	//	{
	//		CallInitialize(static_cast<SceneNode*>((*iter)));
	//	}
	//}

	void Scene::Update(float dt, float et)
	{
		CallUpdate(GetRoot(), dt, et);
	}

	void Scene::CallUpdate(SceneNode* pNode, float dt, float et)
	{
		pNode->Update(dt, et);
		for (auto iter = pNode->_children.begin(); iter != pNode->_children.end(); ++iter)
		{
			CallUpdate(static_cast<SceneNode*>((*iter)), dt, et);
		}
	}

	void Scene::Traverse(TraverseFunc func, void* pUserData) const
	{
		if (func)
		{
			CallTraverse(GetRoot(), func, pUserData);
		}
	}


	void Scene::CallTraverse(SceneNode* pNode, TraverseFunc func, void* pUserData) const
	{
		func(pNode, pUserData);
		for (auto iter = pNode->_children.begin(); iter != pNode->_children.end(); ++iter)
		{
			CallTraverse(static_cast<SceneNode*>((*iter)), func, pUserData);
		}
	}

	void Scene::Clear()
	{
		_root->_children.clear();
		_nodes.clear();
	}
}