#include "SceneNode.h"

namespace SunEngine
{
	const glm::mat4 g_MtxIden = glm::mat4(1.0f);

	SceneNode::SceneNode(Scene* pScene)
	{
		_scene = pScene;
		_worldMatrix = g_MtxIden;
		_localMatrix = g_MtxIden;
		_bInitialized = false;
	}

	SceneNode::~SceneNode()
	{

	}

	void SceneNode::Initialize()
	{
		if (!_bInitialized)
		{
			for (auto iter = _componentList.begin(); iter != _componentList.end(); ++iter)
			{
				(*iter)->Initialize(this, GetComponentData<ComponentData>(*iter));
			}

			_bInitialized = true;
		}
	}

	void SceneNode::UpdateTransform()
	{
		if (this->GetName() == "teddyzsphere_unfold3d")
		{
			int ww = 5;
			ww++;
		}

		_localMatrix = BuildLocalMatrix();

		if (_parent)
		{
			const glm::mat4& mtxParent = GetParent()->GetWorld();
			_worldMatrix = mtxParent * _localMatrix;
		}
		else
			_worldMatrix = _localMatrix;
	}

	void SceneNode::Update(float dt, float et)
	{
		UpdateTransform();

		for (auto iter = _componentList.begin(); iter != _componentList.end(); ++iter)
		{
			(*iter)->Update(this, GetComponentData<ComponentData>(*iter), dt, et);
		}
	}

	SceneNode* SceneNode::GetParent() const
	{
		return static_cast<SceneNode*>(_parent);
	}

	void SceneNode::SetParent(SceneNode* pNode)
	{
		ReParent(pNode);
	}

	ComponentData* SceneNode::GetComponentDataInParent(ComponentType type) const
	{
		SceneNode* parent = GetParent();
		while (parent)
		{
			Vector<Component*> list;
			if (parent->GetComponentsOfType(type, list))
				return parent->_componentDataMap.at(list[0]).get();
			parent = parent->GetParent();
		}

		return 0;
	}

	void SceneNode::OnAddComponent(Component* pComponent)
	{
		_componentList.push_back(pComponent);
		ComponentData* pData = pComponent->AllocData(this);
		if (pData)
		{
			_componentDataMap[pComponent] = UniquePtr<ComponentData>(pData);
		}
	}

	bool SceneNode::Traverse(bool(*NodeFunc)(SceneNode* pNode, void* pData), void* pData)
	{
		if (!NodeFunc(this, pData))
			return false;

		for (auto iter = _children.begin(); iter != _children.end(); ++iter)
		{
			if (!static_cast<SceneNode*>(*iter)->Traverse(NodeFunc, pData))
				return false;
		}

		return true;
	}

	SceneNode::Iter::Iter(const SceneNode* pNode)
	{
		_cur = pNode->_children.begin();
		_end = pNode->_children.end();
	}

	SceneNode* SceneNode::Iter::operator++()
	{
		++_cur;
		return !End() ? static_cast<SceneNode*>(*_cur) : 0;
	}
}