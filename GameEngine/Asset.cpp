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

	SceneNode* Asset::CreateSceneNode(Scene* pScene, float assetScale) const
	{
		//SceneNode* pRoot = pScene->AddNode(GetName());
		AssetNode* pAssetNode = BuildSceneNode(GetRoot(), 0, pScene);

		if (assetScale != 0.0f)
		{
			AABB bounds;
			if (ComputeBoundingVolume(bounds))
			{
				glm::vec3 delta = bounds.Max - bounds.Min;
				if (!isinf(glm::length(delta)))
				{
					float maxAxis = -FLT_MAX;
					maxAxis = glm::max(delta.x, glm::max(delta.y, glm::max(delta.z, maxAxis)));

					AssetNode* pScaleNode = pScene->AddNode("ScaleNode");
					auto childList = pAssetNode->_children;
					for (AssetNode* child : childList)
						child->ReParent(pScaleNode);
					pScaleNode->Scale = glm::vec3(assetScale / maxAxis);
					pScaleNode->ReParent(pAssetNode);
				}
			}
		}

		SceneNode* pSceneNode = static_cast<SceneNode*>(pAssetNode);
		pSceneNode->Traverse([](SceneNode* pNode, void*) -> bool { pNode->Initialize(); return true; });
		return pSceneNode;
	}

	SceneNode* Asset::BuildSceneNode(AssetNode* pCurrNode, AssetNode* pCurrParent, Scene* pScene) const
	{
		SceneNode* pNode = pScene->AddNode(pCurrParent ? pCurrNode->_name : GetName());
		if(pCurrParent)
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

	bool Asset::ComputeBoundingVolume(AABB& aabb) const
	{
		bool expanded = false;
		aabb.Reset();
		for (uint i = 0; i < _nodes.size(); i++)
		{
			AssetNode* pNode = _nodes[i].get();
			glm::mat4 mtx = pNode->BuildWorldMatrix();
			Vector<MeshRenderer*> meshRenderers;
			for (uint j = 0; j < pNode->GetComponentsOfType(meshRenderers); j++)
			{
				AABB box = meshRenderers[j]->GetMesh()->GetAABB();
				box.Transform(mtx);
				aabb.Expand(box);
				expanded = true;
			}
		}

		return expanded;
	}
}