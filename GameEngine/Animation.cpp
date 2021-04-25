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
		_playing = false;
		_loop = true;
		_boneData.resize(boneCount);
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
		auto* data = static_cast<AnimatorComponentData*>(pData);

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
		return new AnimatedBoneComponentData(this, pNode, static_cast<AnimatorComponentData*>(pNode->GetComponentDataInParent(COMPONENT_ANIMATOR)));
	}

	void AnimatedBone::Update(SceneNode* pNode, ComponentData* pData, float, float)
	{
		auto* data = static_cast<AnimatedBoneComponentData*>(pData);

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
		animator->SetBone(_boneIndex, data);
	}

	SkinnedMesh::SkinnedMesh()
	{
		_skinIndex = 0;
	}

	SkinnedMesh::~SkinnedMesh()
	{
	}

	ComponentData* SkinnedMesh::AllocData(SceneNode* pNode)
	{
		return new SkinnedMeshComponentData(this, pNode, static_cast<AnimatorComponentData*>(pNode->GetComponentDataInParent(COMPONENT_ANIMATOR)));
	}
}