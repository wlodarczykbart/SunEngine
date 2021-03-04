#include "SceneNode.h"

namespace SunEngine
{
	glm::mat4 g_MtxIden = glm::mat4(1.0f);

	SceneNode::SceneNode()
	{
		_worldMatrix = g_MtxIden;
		_localMatrix = g_MtxIden;
		_numRenderComponents = 0;
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

	void SceneNode::Update(float dt, float et)
	{
		_localMatrix = BuildLocalMatrix();

		if (_parent)
			_worldMatrix = GetParent()->GetWorld() * _localMatrix;
		else
			_worldMatrix = _localMatrix;

		for (auto iter = _componentList.begin(); iter != _componentList.end(); ++iter)
		{
			(*iter)->Update(this, GetComponentData<ComponentData>(*iter), dt, et);
		}
	}

	SceneNode* SceneNode::GetParent()
	{
		return static_cast<SceneNode*>(_parent);
	}

	void SceneNode::SetParent(SceneNode* pNode)
	{
		ReParent(pNode);
	}

	void SceneNode::OnAddComponent(Component* pComponent)
	{
		if (pComponent->CanRender())
			_numRenderComponents++;

		_componentList.push_back(pComponent);
		ComponentData* pData = pComponent->AllocData();
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