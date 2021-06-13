#include "SceneNode.h"
#include "Animation.h"

namespace SunEngine
{
	AnimatorComponentData::AnimatorComponentData(Component* pComponent, SceneNode* pNode, uint boneCount) : ComponentData(pComponent, pNode)
	{
		_clip = 0;
		_key = 0;
		_time = 0.0f;
		_percent = 0.0f;
		_speed = 1.0f;
		_playing = true;
		_loop = true;
		_boneUpdateCount = 0;
		_boneData.resize(boneCount);
	}

	void AnimatorComponentData::SetClip(uint clip)
	{
		_clip = glm::min(clip, C()->As<Animator>()->GetClipCount() - 1);
	}

	void AnimatorComponentData::RegisterBone(AnimatedBoneComponentData* pBoneData)
	{
		_boneData[pBoneData->C()->As<AnimatedBone>()->GetBoneIndex()] = pBoneData;
	}

	void AnimatorComponentData::IncrementBoneUpdates()
	{
		if (_boneUpdateCount < _boneData.size())
		{
			++_boneUpdateCount;
			if (_boneUpdateCount == _boneData.size())
			{
				//This call will do nothing if the mesh hasn't been updated yet, but if it has then it will want the mesh bones updated
				//because it couldn't do so at the point its update was called due to the bones being located after the mesh in the hierarchy
				for (auto pMesh : _meshData)
					pMesh->UpdateBoneMatrices();
			}
		}
		else
		{
			assert(false);
		}
	}

	glm::mat4 AnimatorComponentData::CalcSkinnedBoneMatrix(uint skinIndex, uint boneIndex) const
	{
		return _boneData[boneIndex]->GetNode()->GetWorld() * _boneData[boneIndex]->C()->As<AnimatedBone>()->GetSkinMatrix(skinIndex);
	}

	Animator::Animator()
	{
		_boneCount = 0;
	}

	Animator::~Animator()
	{
	}

	void Animator::Update(SceneNode*, ComponentData* pData, float dt, float)
	{
		auto* data = pData->As<AnimatorComponentData>();

		data->_boneUpdateCount = 0;
		for (auto pMesh : data->_meshData)
			pMesh->_bUpdateCalled = false;

		if(!data->ShouldUpdate())
			return;

		const AnimationClip& clip = _clips[data->_clip];

		if (data->_time > clip.GetLength())
		{
			data->_key = 0;
			data->_time = 0.0f;
			data->_percent = 0.0f;
			data->_playing = data->_loop;
		}

		if (data->_time > clip.GetKeyTime(data->_key + 1))
			++data->_key;

		float keyTime0 = clip.GetKeyTime(data->_key);
		float keyTime1 = clip.GetKeyTime(data->_key + 1);

		data->_percent = (data->_time - keyTime0) / (keyTime1 - keyTime0);
		data->_time += dt * data->_speed;
	}

	float AnimationClip::GetKeyTime(uint keyIndex) const
	{
		if (keyIndex < _keys.size())
			return _keys[keyIndex];

		return 0.0f;
	}

	void AnimationClip::SetKeys(const Vector<float>& keys)
	{
		_keys = keys;

		for (float& key : _keys)
			key -= _keys[0];
		_length = _keys.size() ? _keys.back() : 0.0f;
	}

	AnimatedBone::AnimatedBone()
	{
		_boneIndex = 0;
	}

	AnimatedBone::~AnimatedBone()
	{

	}

	ComponentData* AnimatedBone::AllocData(SceneNode* pNode)
	{
		AnimatorComponentData* pAnimData = pNode->GetComponentDataInParent(COMPONENT_ANIMATOR)->As<AnimatorComponentData>();
		auto* pBoneData = new AnimatedBoneComponentData(this, pNode);
		pBoneData->_animatorData = pAnimData;
		pAnimData->RegisterBone(pBoneData);
		return pBoneData;
	}

	void AnimatedBone::Update(SceneNode* pNode, ComponentData* pData, float, float)
	{
		auto* data = pData->As<AnimatedBoneComponentData>();

		auto* animator = data->GetAnimatorData();
		if (!animator->ShouldUpdate())
			return;

		uint clip = animator->GetClip();
		uint key0 = animator->GetKey();
		uint key1 = key0 + 1;
		float percent = animator->GetKeyPercent();

		const Transform& t0 = _boneTransforms[clip][key0];
		const Transform& t1 = _boneTransforms[clip][key1];

		pNode->Position = glm::mix(t0.position, t1.position, percent);
		pNode->Scale = glm::mix(t0.scale, t1.scale, percent);
		pNode->Orientation.Quat = glm::slerp(t0.rotation, t1.rotation, percent);
		pNode->Orientation.Mode = ORIENT_QUAT;

		pNode->UpdateTransform();
		animator->IncrementBoneUpdates();
	}

	AnimatedBone::Transform::Transform()
	{
		position = Vec3::Zero;
		scale = Vec3::One;
		rotation = Quat::Identity;
	}

	SkinnedMesh::SkinnedMesh()
	{
		_skinIndex = 0;
		_mesh = 0;
	}

	SkinnedMesh::~SkinnedMesh()
	{
	}

	ComponentData* SkinnedMesh::AllocData(SceneNode* pNode)
	{
		auto pAnimData = pNode->GetComponentDataInParent(COMPONENT_ANIMATOR)->As<AnimatorComponentData>();
		SkinnedMeshComponentData* pMeshData = new SkinnedMeshComponentData(this, pNode);
		pMeshData->_animatorData = pAnimData;
		pAnimData->RegisterMesh(pMeshData);
		pMeshData->_meshBoneMatrices.resize(pAnimData->C()->As<Animator>()->GetBoneCount());
		return pMeshData;
	}

	void SkinnedMesh::Update(SceneNode* pNode, ComponentData* pData, float, float)
	{
		auto data = pData->As<SkinnedMeshComponentData>();

		data->_bUpdateCalled = true;

		//The mesh is located after the skeleton hierarchy, need to update toe bones now as the call to UpdateBoneMatrices in the animator wouldn't have done anything
		//due to the mesh not being updated yet
		if (data->_animatorData->GetBoneUpdateCount() == data->_meshBoneMatrices.size())
			data->UpdateBoneMatrices();
	}

	SkinnedMeshComponentData::SkinnedMeshComponentData(Component* pComponent, SceneNode* pNode) : ComponentData(pComponent, pNode)
	{
		_bUpdateCalled = false;
		_animatorData = 0;
	}

	void SkinnedMeshComponentData::UpdateBoneMatrices()
	{
		if (_bUpdateCalled)
		{
			glm::mat4 invMtx = glm::inverse(GetNode()->GetWorld());
			const SkinnedMesh* pMesh = C()->As <SkinnedMesh>();
			for (uint i = 0; i < _meshBoneMatrices.size(); i++)
				_meshBoneMatrices[i] = invMtx * _animatorData->CalcSkinnedBoneMatrix(pMesh->GetSkinIndex(), i);
		}
	}
}