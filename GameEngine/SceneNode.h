#pragma once

#include "AssetNode.h"
#include "MemBuffer.h"

namespace SunEngine
{
	class Scene;

	class SceneNode final : public AssetNode
	{
	public:
		class Iter
		{
		public:
			explicit Iter(const SceneNode* pNode);

			SceneNode* operator *() const { return static_cast<SceneNode*>(*_cur); };
			SceneNode* operator ++();
			bool End() const { return _cur == _end; }

		private:
			LinkedList<AssetNode*>::const_iterator _cur;
			LinkedList<AssetNode*>::const_iterator _end;
		};

		SceneNode(Scene* pScene);
		SceneNode(const SceneNode&) = delete;
		SceneNode& operator = (const SceneNode&) = delete;
		virtual ~SceneNode();

		void UpdateTransform();

		virtual void Initialize();
		virtual void Update(float dt, float et);

		const glm::mat4& GetWorld() const { return _worldMatrix; }

		SceneNode* GetParent() const;
		void SetParent(SceneNode* pNode);

		LinkedList<Component*>::const_iterator BeginComponent() const { return _componentList.begin(); }
		LinkedList<Component*>::const_iterator EndComponent() const { return _componentList.end(); }

		Iter GetChildIterator() const { return Iter(this); }

		bool Traverse(bool(*NodeFunc)(SceneNode* pNode, void* pData), void* pData = 0);

		template<typename T>
		T* GetComponentData(Component* pComponent) const
		{
			auto found = _componentDataMap.find(pComponent);
			return found == _componentDataMap.end() ? 0 : static_cast<T*>((*found).second.get());
		}

		ComponentData* GetComponentDataInParent(ComponentType type) const;

		bool CanRender() const { return _numRenderComponents > 0; }

		Scene* GetScene() const { return _scene; }

	private:
		void OnAddComponent(Component* pComponent) override;

		friend class Scene;

		Scene* _scene;
		LinkedList<Component*> _componentList;
		Map<Component*, UniquePtr<ComponentData>> _componentDataMap;
		glm::mat4 _localMatrix;
		glm::mat4 _worldMatrix;
		uint _numRenderComponents;
		bool _bInitialized;
	};
}