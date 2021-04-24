//#include <DirectXCollision.h>
#include "StringUtil.h"
#include "RenderObject.h"
#include "Mesh.h"
#include "Scene.h"

#define SCENE_ROOT_NAME "SceneRoot"

namespace SunEngine
{
	Scene::Scene()
	{
		_root = UniquePtr<SceneNode>(new SceneNode(this));
		_root->_name = SCENE_ROOT_NAME;
	}

	Scene::~Scene()
	{
	}

	SceneNode* Scene::AddNode(const String& name)
	{
		uint counter = 0;
		String uniqueName = name;
		while (_nodes.find(uniqueName) != _nodes.end())
		{
			uniqueName = StrFormat("%s_%d", name.c_str(), ++counter);
		}

		SceneNode* pNode = new SceneNode(this);
		pNode->_name = uniqueName;
		_nodes[uniqueName] = UniquePtr<SceneNode>(pNode);

		pNode->SetParent(GetRoot());
		return pNode;
	}

	SceneNode* Scene::GetNode(const String& name) const
	{
		auto found = _nodes.find(name);
		return found != _nodes.end() ? (*found).second.get() : 0;
	}

	bool Scene::RemoveNode(SceneNode* pNode)
	{
		if (!pNode)
			return false;

		if (pNode == GetRoot())
			return false;

		auto found = _nodes.find(pNode->GetName());
		if (found != _nodes.end() && (*found).second.get() == pNode)
		{
			pNode->SetParent(0);
			LinkedList<SceneNode*> nodesToRemove;
			pNode->Traverse([](SceneNode* pRemoveNode, void* pNodeMap) -> bool {
				static_cast<LinkedList<SceneNode*>*>(pNodeMap)->push_front(pRemoveNode);
				return true;
			}, & nodesToRemove);

			for (auto iter = nodesToRemove.begin(); iter != nodesToRemove.end(); ++iter)
			{
				SceneNode* pRemoveNode = *iter;
				for (auto citer = pRemoveNode->BeginComponent(); citer != pRemoveNode->EndComponent(); ++citer)
				{
					Component* component = (*citer);
					if (component->CanRender())
					{
						RenderComponentData* pRenderData = pRemoveNode->GetComponentData<RenderComponentData>(component);
						for (auto renderIter = pRenderData->BeginNode(); renderIter != pRenderData->EndNode(); ++renderIter)
						{
							const RenderNode* pRenderNode = &(*renderIter);
							_renderNodes.erase(pRenderNode);
						}
					}
				}

				_nodes.erase(pRemoveNode->GetName());
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	//void Scene::Initialize()
	//{
	//	CallInitialize(GetRoot());
	//}

	//void Scene::CallInitialize(SceneNode* pNode)
	//{
	//	pNode->Initialize();
	//	for (auto iter = pNode->_children.begin(); iter != pNode->_children.end(); ++iter)
	//	{
	//		CallInitialize(static_cast<SceneNode*>((*iter)));
	//	}
	//}

	void Scene::Update(float dt, float et)
	{
		CallUpdate(GetRoot(), dt, et);
		UpdateRenderNodes();
	}

	void Scene::CallUpdate(SceneNode* pNode, float dt, float et)
	{
		pNode->Update(dt, et);
		for (auto iter = pNode->_children.begin(); iter != pNode->_children.end(); ++iter)
		{
			CallUpdate(static_cast<SceneNode*>((*iter)), dt, et);
		}
	}

	void Scene::Traverse(TraverseFunc func, void* pUserData) const
	{
		if (func)
		{
			CallTraverse(GetRoot(), func, pUserData);
		}
	}


	void Scene::CallTraverse(SceneNode* pNode, TraverseFunc func, void* pUserData) const
	{
		func(pNode, pUserData);
		for (auto iter = pNode->_children.begin(); iter != pNode->_children.end(); ++iter)
		{
			CallTraverse(static_cast<SceneNode*>((*iter)), func, pUserData);
		}
	}

	void Scene::UpdateRenderNodes()
	{
		for (auto& kv : _renderNodes)
		{
			auto pNode = kv.first;
			auto data = &kv.second;
			if (pNode->GetWorld() != data->mtx || pNode->GetAABB() != data->aabb) //assume sphere changes when aabb changes
			{
				data->mtx = pNode->GetWorld();
				data->invMtx = glm::inverse(data->mtx);
				data->aabb = pNode->GetAABB();
				data->sphere = pNode->GetSphere();
			}
		}
	}

	void Scene::Clear()
	{
		_root->_children.clear();
		_nodes.clear();
	}

	void Scene::RegisterRenderNode(RenderNode* pNode)
	{
		auto found = _renderNodes.find(pNode);
		RenderNodeData* data = 0;
		if (found == _renderNodes.end())
		{
			data = &_renderNodes[pNode];
		}
		else
		{
			data = &(*found).second;
		}

		data->pNode = pNode;
		data->mtx = pNode->GetWorld();
		data->aabb = pNode->GetAABB();
		data->sphere = pNode->GetSphere();
	}

	bool Scene::Raycast(const glm::vec3& o, const glm::vec3& d, SceneRayHit& hit) const
	{
		hit.pHitNode = NULL;
#if 1
		//hit.numHits = 0;
		hit.pHitNode = 0;
		for (auto& kv : _renderNodes)
		{
			auto& data = kv.second;

			Ray ray;
			ray.Origin = o;
			ray.Direction = d;
			ray.Transform(data.invMtx);

			bool hitSphere = RaySphereIntersect(ray, data.sphere.Center, data.sphere.Radius);
			bool hitAABB = RayAABBIntersect(ray, data.aabb.Min, data.aabb.Max);

			if (hitSphere && hitAABB)
			{
				 Mesh* pMesh = data.pNode->GetMesh();
				 auto& vtxDef = pMesh->GetVertexDef();
				 uint nIndex = vtxDef.NormalIndex;
				 float tMin = FLT_MAX;

				 for (uint i = 0; i < pMesh->GetTriCount(); i++)
				 {
					 uint t0, t1, t2;
					 pMesh->GetTri(i, t0, t1, t2);

					 glm::vec3 p0, p1, p2;
					 p0 = pMesh->GetVertexPos(t0);
					 p1 = pMesh->GetVertexPos(t1);
					 p2 = pMesh->GetVertexPos(t2);

					 float w[3];
					 if (RayTriangleIntersect(ray, p0, p1, p2, tMin, w))
					 {
						 hit.pHitNode = data.pNode;
						 hit.position = p0 * w[0] + p1 * w[1] + p2 * w[2];
						 if (nIndex != VertexDef::DEFAULT_INVALID_INDEX)
						 {
							 hit.normal = pMesh->GetVertexVar(t0, nIndex) * w[0] + pMesh->GetVertexVar(t1, nIndex) * w[1] + pMesh->GetVertexVar(t2, nIndex) * w[2];
						 }
						 break;
					 }
				 }
			}

			//float r2 = glm::dot(halfExtent, halfExtent);
			//glm::vec3 rayToCenter = center - o;

			//float a2 = glm::dot(d, rayToCenter);

			////DirectX::BoundingSphere sphere;
			////sphere.Center = DirectX::XMFLOAT3(center.x, center.y, center.z);
			////sphere.Radius = radius;

			////DirectX::FXMVECTOR dx_o = DirectX::XMVectorSet(o.x, o.y, o.z, 1.0f);
			////DirectX::FXMVECTOR dx_d = DirectX::XMVectorSet(d.x, d.y, d.z, 0.0f);

			////float dist;
			////if (sphere.Intersects(dx_o, dx_d, dist))
			////	hit.numHits++;

			//if (a2 < 0.0f)
			//	continue;

			//a2 = a2 * a2;
			//float c2 = glm::dot(rayToCenter, rayToCenter);
			//float b2 = (c2 - a2);

			//if (b2 > r2)
			//	continue;

			//hit.numHits++;
		}
#endif

		return hit.pHitNode != NULL;
	}
}