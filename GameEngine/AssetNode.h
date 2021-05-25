#pragma once

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include "Types.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

namespace SunEngine
{
	class SceneNode;

	enum ComponentType
	{
		COMPONENT_CAMERA,
		COMPONENT_LIGHT,
		COMPONENT_RENDER_OBJECT,
		COMPONENT_ANIMATOR,
		COMPONENT_ANIMATED_BONE,
		COMPONENT_SKINNED_MESH,
		COMPONENT_ENVIRONMENT,
		COMPONENT_COUNT,
	};

	class Component;

	class ComponentData
	{
	public:
		ComponentData(Component* pComponent, SceneNode* pNode) : _c(pComponent), _node(pNode) { }
		virtual ~ComponentData() {}
		
		const Component* C() const { return _c;  }
		SceneNode* GetNode() const { return _node; }

		template<typename T>
		T* As()
		{
			return static_cast<T*>(this);
		}

		template<typename T>
		const T* As() const
		{
			return static_cast<const T*>(this);
		}
	private:
		Component* _c;
		SceneNode* _node;
	};

	class Component
	{
	public:
		Component() {}
		Component(const Component&) = delete;
		Component& operator = (const Component&) = delete;
		virtual ~Component() {};

		virtual ComponentType GetType() const = 0;
		virtual ComponentData* AllocData(SceneNode*) { return 0; }

		virtual Component* GetBase() { return this; }
		virtual const Component* GetBase() const { return this; }

		virtual void Initialize(SceneNode* pNode, ComponentData* pData);
		virtual void Update(SceneNode* pNode, ComponentData* pData, float dt, float et);
		template<typename T>
		T* As()
		{
			return static_cast<T*>(GetBase());
		}

		template<typename T>
		const T* As() const
		{
			return static_cast<const T*>(GetBase());
		}
	};

	class ComponentRef : public Component
	{
	public:
		ComponentRef(Component* pBase) { _base = pBase; }
		~ComponentRef() {}

		ComponentType GetType() const override { return _base->GetType(); };
		ComponentData* AllocData(SceneNode* pNode) override { return _base->AllocData(pNode); }
		Component* GetBase() override { return _base; }
		const Component* GetBase() const override { return _base; }
		void Initialize(SceneNode* pNode, ComponentData* pData) override  { _base->Initialize(pNode, pData); }
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override { _base->Update(pNode, pData, dt, et); }
	private:
		ComponentRef() = delete;

		Component* _base;
	};

	enum OrientationMode
	{
		ORIENT_XYZ,
		ORIENT_XZY,
		ORIENT_YXZ,
		ORIENT_YZX,
		ORIENT_ZXY,
		ORIENT_ZYX,
		ORIENT_QUAT,
	};

	class Orientation
	{
	public:
		void Reset();
		glm::mat4 BuildMatrix() const;

		OrientationMode Mode;
		glm::vec3 Angles;
		glm::quat Quat;
	};

	class AssetNode
	{
	public:
		AssetNode();
		AssetNode(const AssetNode&) = delete;
		AssetNode& operator = (const AssetNode&) = delete;
		~AssetNode();

		Component* AddComponent(Component* pComponent);

		uint GetComponentCount(ComponentType type) const { return _components.at(type).size(); }
		const String& GetName() const { return _name; }

		glm::vec3 Position;
		glm::vec3 Scale;
		Orientation Orientation;

		glm::mat4 BuildLocalMatrix() const;
		glm::mat4 BuildWorldMatrix() const;

		void SetVisible(bool visible);
		bool GetVisible() const { return _visible; }
		bool GetTotalVisibility() const { return _visible && _parentVisible; }

		template<typename T>
		uint GetComponentsOfType(Vector<T*>& components) const
		{
			ComponentType type = T::CType;
			for (uint i = 0; i < _components[type].size(); i++)
				components.push_back(_components[type][i]->As<T>());

			return _components[type].size();
		}

		uint GetComponentsOfType(ComponentType type, Vector<Component*>& components) const;
		Component* GetComponentOfType(ComponentType type) const;

	protected:
		void SetParentVisible(bool parentVisible);

		void ReParent(AssetNode* pParent);
		virtual void OnAddComponent(Component*) {};

		friend class Asset;
		String _name;
		bool _visible;
		bool _parentVisible;
		AssetNode* _parent;
		LinkedList<AssetNode*> _children;
		Vector<Vector<UniquePtr<Component>>> _components;
	};
}