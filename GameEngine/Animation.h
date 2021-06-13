#pragma once

#include "AssetNode.h"

namespace SunEngine
{
	class Mesh;

	class AnimationClip
	{
	public:
		uint GetKeyCount() const { return _keys.size(); }
		float GetKeyTime(uint keyIndex) const;
		float GetLength() const { return _length; }

		void SetName(const String& name) { _name = name; }
		void SetKeys(const Vector<float>& keys);
		void GetKeys(Vector<float>& keys) const { keys = _keys; }

	private:
		friend class Animator;

		String _name;
		float _length;
		Vector<float> _keys;
	};

	class AnimatedBoneComponentData;
	class SkinnedMeshComponentData;

	class AnimatorComponentData : public ComponentData
	{
	public:
		AnimatorComponentData(Component* pComponent, SceneNode* pNode, uint boneCount);

		uint GetClip() const { return _clip; }
		void SetClip(uint clip);

		uint GetKey() const { return _key; }
		float GetKeyPercent() const { return _percent; }

		bool GetPlaying() const { return _playing; }
		void SetPlaying(bool playing) { _playing = playing; }

		bool GetLoop() const { return _loop; }
		void SetLoop(bool loop) { _loop = loop; }

		float GetSpeed() const { return _speed; }
		void SetSpeed(float speed) { _speed = speed; }

		bool ShouldUpdate() const { return _playing; }

		void RegisterBone(AnimatedBoneComponentData* pBoneData);
		uint GetBoneCount() const { return _boneData.size(); }

		void IncrementBoneUpdates();
		uint GetBoneUpdateCount() const { return _boneUpdateCount; }

		void RegisterMesh(SkinnedMeshComponentData* pMeshData) { _meshData.push_back(pMeshData); }

		glm::mat4 CalcSkinnedBoneMatrix(uint skinIndex, uint boneIndex) const;

	private:
		friend class Animator;

		uint _clip;
		uint _key;
		float _time;
		float _percent;
		float _speed;
		bool _playing;
		bool _loop;
		uint _boneUpdateCount;

		Vector<AnimatedBoneComponentData*> _boneData;
		Vector<SkinnedMeshComponentData*> _meshData;
	};

	class Animator : public Component
	{
	public:
		Animator();
		~Animator();

		ComponentType GetType() const override { return COMPONENT_ANIMATOR; }
		ComponentData* AllocData(SceneNode* pNode) override { return new AnimatorComponentData(this, pNode, _boneCount); }

		uint GetClipCount() const { return _clips.size(); }
		void SetClips(const Vector<AnimationClip>& clips) { _clips = clips; }

		uint GetBoneCount() const { return _boneCount; }
		void SetBoneCount(uint count) { _boneCount = count; }

		void GetClipKeys(uint clipIndex, Vector<float>& keys) const { if (clipIndex < _clips.size()) _clips[clipIndex].GetKeys(keys); }

		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;
	private:
		uint _boneCount;
		Vector<AnimationClip> _clips;
	};

	class AnimatedBoneComponentData : public ComponentData
	{
	public:
		AnimatedBoneComponentData(Component* pComponent, SceneNode* pNode) : ComponentData(pComponent, pNode) { _animatorData = 0; }
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
			Transform();

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
		SkinnedMeshComponentData(Component* pComponent, SceneNode* pNode);
		AnimatorComponentData* GetAnimatorData() const { return _animatorData; }

		const glm::mat4* GetMeshBoneMatrices() const { return _meshBoneMatrices.data(); }
		void UpdateBoneMatrices();
	private:

		friend class SkinnedMesh;
		friend class Animator;
		AnimatorComponentData* _animatorData;
		Vector<glm::mat4> _meshBoneMatrices;
		bool _bUpdateCalled;
	};

	class SkinnedMesh : public Component
	{
	public:
		SkinnedMesh();
		~SkinnedMesh();

		ComponentType GetType() const override { return COMPONENT_SKINNED_MESH; }
		ComponentData* AllocData(SceneNode* pNode) override;

		void SetMesh(Mesh* pMesh) { _mesh = pMesh; }
		Mesh* GetMesh() const { return _mesh; }

		void SetSkinIndex(uint skinIndex) { _skinIndex = skinIndex; }
		uint GetSkinIndex() const { return _skinIndex; }

		void Update(SceneNode* pNode, ComponentData* pData, float dt, float et) override;
	private:
		Mesh* _mesh;
		uint _skinIndex;
	};
}