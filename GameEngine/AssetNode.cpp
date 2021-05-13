#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "AssetNode.h"

namespace SunEngine
{
	const glm::mat4 g_MtxIden = glm::mat4(1.0f);
	/*
		ORIENT_XYZ,
		ORIENT_XZY,
		ORIENT_YXZ,
		ORIENT_YZX,
		ORIENT_ZXY,
		ORIENT_ZYX,
	*/

	const glm::ivec3 g_RotOrderTable[]
	{
		glm::ivec3(0, 1, 2),  //ORIENT_XYZ,
		glm::ivec3(0, 2, 1),  //ORIENT_XZY,
		glm::ivec3(1, 0, 2),  //ORIENT_YXZ,
		glm::ivec3(1, 2, 0),  //ORIENT_YZX,
		glm::ivec3(2, 0, 1),  //ORIENT_ZXY,
		glm::ivec3(2, 1, 0),  //ORIENT_ZYX,
	};


	AssetNode::AssetNode()
	{
		_parent = 0;
		Position = glm::vec3(0.0f);
		Scale = glm::vec3(1.0f);
		Orientation.Reset();

		_components.resize(COMPONENT_COUNT);
	}

	AssetNode::~AssetNode()
	{

	}

	Component* AssetNode::AddComponent(Component* pComponent)
	{
		if (pComponent)
		{
			_components.at(pComponent->GetType()).push_back(UniquePtr<Component>(pComponent));
			OnAddComponent(pComponent);
		}
		return pComponent;
	}

	glm::mat4 AssetNode::BuildLocalMatrix() const
	{
		glm::mat4 mtxIden(1.0f);
		glm::mat4 mtxRot = Orientation.BuildMatrix();
		glm::mat4 mtxScale = glm::scale(mtxIden, Scale);
		glm::mat4 mtxTrans = glm::translate(mtxIden, Position);
		return mtxTrans * mtxRot * mtxScale;
	}

	glm::mat4 AssetNode::BuildWorldMatrix() const
	{
		glm::mat4 mtxWorld = BuildLocalMatrix();
		auto parent = _parent;
		while (parent)
		{
			mtxWorld = parent->BuildLocalMatrix() * mtxWorld;
			parent = parent->_parent;
		}
		return mtxWorld;
	}

	void AssetNode::ReParent(AssetNode* pParent)
	{
		if (_parent) _parent->_children.remove(this);
		_parent = pParent;
		if (_parent) _parent->_children.push_back(this);
	}

	uint AssetNode::GetComponentsOfType(ComponentType type, Vector<Component*>& components) const
	{
		for (uint i = 0; i < _components[type].size(); i++)
			components.push_back(_components[type][i].get());

		return _components[type].size();
	}

	Component* AssetNode::GetComponentOfType(ComponentType type) const
	{
		return _components[type].size() ? _components[type].front().get() : 0;
	}

	void Component::Initialize(SceneNode*, ComponentData*)
	{

	}

	void Component::Update(SceneNode*, ComponentData*, float, float)
	{

	}

	void Orientation::Reset()
	{
		Mode = ORIENT_XYZ;
		Angles = glm::vec3(0.0f);
		Quat = glm::quat();
	}

	glm::mat4 Orientation::BuildMatrix() const
	{
		if (Mode == ORIENT_QUAT)
		{
			return glm::toMat4(Quat);
		}
		else
		{
			glm::mat4 mtx = glm::mat4(1.0f);
			glm::vec3 anglesRad = glm::radians(Angles);
			glm::ivec3 rotOrder = g_RotOrderTable[Mode];

			glm::mat4 mtxTable[3];
			mtxTable[0] = glm::eulerAngleX(anglesRad.x);
			mtxTable[1] = glm::eulerAngleY(anglesRad.y);
			mtxTable[2] = glm::eulerAngleZ(anglesRad.z);

			return mtxTable[rotOrder.z] * mtxTable[rotOrder.y] * mtxTable[rotOrder.x];
		}
	}
}