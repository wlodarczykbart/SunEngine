#pragma once

#include "AssetNode.h"

namespace SunEngine
{
	class AnimationClip
	{
	public:
		uint GetKeyCount() const { return _keys.size(); }
		float GetKeyTime(uint keyIndex) const;
		float GetLength() const { return _length; }

		void SetName(const String& name) { _name = name; }
		void SetKeys(const Vector<float>& keys);

	private:
		friend class Aniamtor;

		String _name;
		float _length;
		Vector<float> _keys;
	};

	class AnimatedBoneComponentData;

	class AnimatorComponentData : public ComponentData
	{
	public:
		AnimatorComponentData(Component* pComponent, SceneNode* pNode, uint boneCount);

		uint GetClip() const { return _clip; }
		uint GetKey() const { return _key; }
		float GetKeyPercent() const { return _percent; }
		bool ShouldUpdate() const { return _playing; }
		bool GetPlaying() const { return _playing; }
		void SetPlaying(bool playing) { _playing = playing; }

		void SetBone(uint index, const AnimatedBoneComponentData* pBoneData) { _boneData[index] = pBoneData; }
		const Vector<const AnimatedBoneComponentData*>& GetBoneData() const { return _boneData; }
		uint GetBoneCount() const { return _boneData.size(); }

	private:
		friend class Animator;

		uint _clip;
		uint _key;
		float _time;
		float _percent;
		float _speed;
		bool _playing;
		bool _loop;

		Vector<const AnimatedBoneComponentData*> _boneData;
	};

	class Animator : public Component
	{
	public:
		Animator();
		~Animator();

		ComponentType GetType() const override { return COMPONENT_ANIMATOR; }
		ComponentData* AllocData(SceneNode* pNode) override { return new AnimatorComponentData(this, pNode, _boneCount); }

		void SetClips(const Vector<AnimationClip>& clips) { _clips = clips; }
		void SetBoneCount(uint count) { _boneCount = count; }
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;
	private:
		uint _boneCount;
		Vector<AnimationClip> _clips;
	};

	class AnimatedBoneComponentData : public ComponentData
	{
	public:
		AnimatedBoneComponentData(Component* pComponent, SceneNode* pNode, AnimatorComponentData* pAnimatorData) : ComponentData(pComponent, pNode) { _animatorData = pAnimatorData; }
		AnimatorComponentData* GetAnimatorData() const { return _animatorData; }

	private:
		friend class AnimatedBone;
		AnimatorComponentData* _animatorData;
	};

	class AnimatedBone : public Component
	{
	public:
		struct Transform
		{
			glm::vec3 position;
			glm::vec3 scale;
			glm::quat rotation;
		};

		AnimatedBone();
		~AnimatedBone();

		ComponentType GetType() const override { return COMPONENT_ANIMATED_BONE; }
		ComponentData* AllocData(SceneNode* pNode) override;

		void SetBoneIndex(uint boneIndex) { _boneIndex = boneIndex; }
		uint GetBoneIndex() const { return _boneIndex; }

		void SetSkinMatrices(const Vector<glm::mat4>& matrices) { _skinMatrices = matrices; }
		const glm::mat4& GetSkinMatrix(const uint index) const { return _skinMatrices.at(index); }

		void SetTransforms(const Vector<Vector<Transform>>& transforms) { _boneTransforms = transforms; }
		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;

	private:
		uint _boneIndex;
		Vector<glm::mat4> _skinMatrices;
		Vector<Vector<Transform>> _boneTransforms;
	};

	//Likely will want to use this to compute a skinned bounding box on the cpu for the time being...
	class SkinnedMeshComponentData : public ComponentData
	{
	public:
		SkinnedMeshComponentData(Component* pComponent, SceneNode* pNode, AnimatorComponentData* pAnimatorData) : ComponentData(pComponent, pNode) { _animatorData = pAnimatorData; }
		AnimatorComponentData* GetAnimatorData() const { return _animatorData; }

	private:
		friend class SkinnedMesh;
		AnimatorComponentData* _animatorData;
	};

	class SkinnedMesh : public Component
	{
	public:
		SkinnedMesh();
		~SkinnedMesh();

		ComponentType GetType() const override { return COMPONENT_SKINNED_MESH; }
		ComponentData* AllocData(SceneNode* pNode) override;

		void SetSkinIndex(uint skinIndex) { _skinIndex = skinIndex; }
		uint GetSkinIndex() const { return _skinIndex; }

	private:
		uint _skinIndex;
	};
}