#include "Scene.h"
#include "MeshRenderer.h"
#include "Mesh.h"
#include "Asset.h"

namespace SunEngine
{
	AssetNode* Asset::AddNode(const String& name)
	{
		AssetNode* pNode = new AssetNode();
		pNode->_name = name;
		_nodes.push_back(UniquePtr<AssetNode>(pNode));
		return pNode;
	}

	AssetNode* Asset::GetNodeByName(const String& name) const
	{
		for (uint i = 0; i < _nodes.size(); i++)
			if (_nodes[i]->_name == name)
				return _nodes[i].get();

		return 0;
	}

	bool Asset::SetParent(const String& child, const String& parent)
	{
		AssetNode* pChild = GetNodeByName(child);
		AssetNode* pParent = GetNodeByName(parent);

		if (pChild && pParent)
		{
			pChild->ReParent(pParent);
			return true;
		}
		else
		{
			return false;
		}
	}

	SceneNode* Asset::CreateSceneNode(Scene* pScene) const
	{
		SceneNode* pRoot = pScene->AddNode(GetName());
		SceneNode* pOldRoot = BuildSceneNode(GetRoot(), pRoot, pScene);

		//glm::vec3 delta = _aabb.Max - _aabb.Min;
		//if (!isinf(glm::length(delta)))
		//{
		//	float maxAxis = -FLT_MAX;
		//	maxAxis = glm::max(delta.x, glm::max(delta.y, glm::max(delta.z, maxAxis)));
		//	pOldRoot->Scale *= 2.0f / maxAxis;
		//}

		pRoot->Traverse([](SceneNode* pNode, void*) -> bool { pNode->Initialize(); return true; });
		return pRoot;
	}

	SceneNode* Asset::BuildSceneNode(AssetNode* pCurrNode, AssetNode* pCurrParent, Scene* pScene) const
	{
		SceneNode* pNode = pScene->AddNode(pCurrNode->_name);
		pNode->ReParent(pCurrParent);
		
		pNode->Position = pCurrNode->Position;
		pNode->Scale = pCurrNode->Scale;
		pNode->Orientation = pCurrNode->Orientation;

		for (uint i = 0; i < COMPONENT_COUNT; i++)
		{
			for (uint j = 0; j < pCurrNode->_components.at(i).size(); j++)
			{
				pNode->AddComponent(new ComponentRef(pCurrNode->_components.at(i).at(j).get()));
			}
		}

		for (auto iter = pCurrNode->_children.begin(); iter != pCurrNode->_children.end(); ++iter)
		{
			BuildSceneNode((*iter), pNode, pScene);
		}

		return pNode;
	}

	void Asset::UpdateBoundingVolume()
	{
		_aabb.Reset();
		for (uint i = 0; i < _nodes.size(); i++)
		{
			AssetNode* pNode = _nodes[i].get();
			glm::mat4 mtx = pNode->BuildWorldMatrix();
			Vector<MeshRenderer*> meshRenderers;
			for (uint j = 0; j < pNode->GetComponentsOfType(meshRenderers); j++)
			{
				AABB box = meshRenderers[j]->GetMesh()->GetAABB();
				box.Transform(mtx);
				_aabb.Expand(box);
			}
		}
	}
}