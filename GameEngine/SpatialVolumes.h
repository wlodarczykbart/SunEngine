#pragma once

#include "ThreadPool.h"

namespace SunEngine
{
	template<typename T>
	class IBoxTree
	{
	public:
		struct Node
		{
			void Reset()
			{
				parent = 0;
				children = 0;
				objects.clear();
			}

			AABB box;
			Node* parent;
			Node* children;
			LinkedList<T> objects;
		};

		typedef bool(*TraverseFunc)(const Node& node, void* pData);
		typedef bool(*TraverseThreadedFunc)(const Node& node, void* pData, uint threadIndex);

		IBoxTree(uint childCount)
		{
			_depth = 0;
			_objectCount = 0;
			_childCount = childCount;
			_nodeCount = 0;
		}

		void Create(const AABB& box, uint depth)
		{
			_depth = depth;

			_objectCount = 0;	
			_nodeCount = 0;
			for (uint i = 0; i <= depth; i++)
				_nodeCount += (uint)powf((float)_childCount, (float)i);

			_nodes.resize(_nodeCount);
			for (uint i = 0; i < _nodeCount; i++)
			{
				_nodes[i].Reset();
			}

			uint nodeOffset = 1;
			Create(GetRoot(), box, (int)depth, nodeOffset);
		}

		const AABB& GetAABB() const
		{
			return GetRoot()->box;
		}

		bool Insert(const T& object, const AABB& box)
		{
			if (Insert(GetRoot(), object, box))
			{
				++_objectCount;
				return true;
			}
			else
			{
				return false;
			}
		}

		bool Remove(const T& object)
		{
			bool done = false;
			Remove(GetRoot(), object, done);
			if (done)
			{
				--_objectCount;
				return true;
			}
			else
			{
				return false;
			}
		}

		void Traverse(TraverseFunc Func, void* pData = 0) const
		{
			if (Func == 0)
				return;

			Traverse(GetRoot(), Func, pData);
		}

		void TraverseThreaded(TraverseThreadedFunc Func, void* pData = 0) const
		{
			if (Func == 0)
				return;

			if (GetRoot() == 0)
				return;

			Func(*GetRoot(), pData, 0);
			ThreadPool& tp = ThreadPool::Get();

			struct ThreadData
			{
				const IBoxTree* pTree;
				uint childIndex;
				TraverseThreadedFunc func;
				void* pData;
			} threadData[8];

			Pair<TraverseThreadedFunc, void*> dataPair = { Func, pData };
			for (uint i = 0; i < _childCount; i++)
			{
				threadData[i].pTree = this;
				threadData[i].childIndex = i;
				threadData[i].func = Func;
				threadData[i].pData = pData;
				tp.AddTask([](uint threadIndex, void* pThreadDataPtr) -> void {
					auto* pThreadData = static_cast<ThreadData*>(pThreadDataPtr);
					pThreadData->pTree->Traverse(&pThreadData->pTree->GetRoot()->children[pThreadData->childIndex], pThreadData->func, pThreadData->pData, threadIndex);
				}, &threadData[i]);
			};

			tp.Wait();
		}

		uint GetObjectCount() const { return _objectCount; }

	private:
		virtual AABB BuildChildBox(uint index, const AABB& parentBox) const = 0;

		void Create(Node* node, const AABB& box, int depth, uint& nodeOffset)
		{
			node->box = box;
	
			//char spacing[64] = {};
			//for (int i = 0; i < _depth - depth; i++)
			//	spacing[i] = '\t';

			//printf("%s%f %f %f    %f %f %f\n", spacing, box.Min.x, box.Min.y, box.Min.z, box.Max.x, box.Max.y, box.Max.z);
			if (depth != 0)
			{
				node->children = &_nodes[nodeOffset];
				nodeOffset += _childCount;
				for (uint i = 0; i < _childCount; i++)
				{
					node->children[i].parent = node;
					Create(&node->children[i], BuildChildBox(i, box), depth - 1, nodeOffset);
				}
			}
		}

		bool Insert(Node* node, const T& object, const AABB& box)
		{
			if (node->box.Contains(box))
			{
				if (node->children)
				{
					for (uint i = 0; i < _childCount; i++)
					{
						if (Insert(&node->children[i], object, box))
							return true;
					}
				}
				node->objects.push_back(object);
				return true;
			}
			else
			{
				return false;
			}
		}

		void Remove(Node* node, const T& object, bool& done)
		{
			if (done)
				return;

			auto found = std::find(node->objects.begin(), node->objects.end(), object);
			if (found != node->objects.end())
			{
				node->objects.erase(found);
				done = true;
			}
		}

		void Traverse(const Node* node, TraverseFunc Func, void* pUserData) const
		{
			if (!Func(*node, pUserData))
				return;

			if (node->children)
			{
				for (uint i = 0; i < _childCount; i++)
					Traverse(&node->children[i], Func, pUserData);
			}
		}

		void Traverse(const Node* node, TraverseThreadedFunc Func, void* pUserData, uint threadIndex) const
		{
			if (!Func(*node, pUserData, threadIndex))
				return;

			if (node->children)
			{
				for (uint i = 0; i < _childCount; i++)
					Traverse(&node->children[i], Func, pUserData, threadIndex);
			}
		}

	protected:
		Node* GetRoot()
		{
			return _nodes.data();
		}

		const Node* GetRoot() const
		{
			return _nodes.data();
		}

		uint _depth;
		uint _objectCount;
		uint _childCount;
		uint _nodeCount;
		Vector<Node> _nodes;
	};

	template<typename T>
	class OctTree : public IBoxTree<T>
	{
	public:
		OctTree() : IBoxTree(8)
		{

		}

	private:
		AABB BuildChildBox(uint index, const AABB& parentBox) const override
		{
			glm::vec3 signs;
			signs.x = (index & 1) ? 1.0f : -1.0f;
			signs.y = (index & 2) ? 1.0f : -1.0f;
			signs.z = (index & 4) ? 1.0f : -1.0f;

			glm::vec3 step = parentBox.GetExtent() * 0.5f;
			glm::vec3 center = parentBox.GetCenter() + step * signs;

			return AABB(center - step, center + step);
		}
	};

	template<typename T>
	class QuadTree : public IBoxTree<T>
	{
	public:
		QuadTree() : IBoxTree(4)
		{

		}

		void ComputeHeights(void(*GetMinMaxHeightFunc)(const T& object, float&, float&))
		{
			for (uint i = 0; i < _nodeCount; i++)
			{
				_nodes[i].box.Min.y = FLT_MAX;
				_nodes[i].box.Max.y = -FLT_MAX;

				for (auto& obj : _nodes[i].objects)
				{
					float oMin, oMax;
					GetMinMaxHeightFunc(obj, oMin, oMax);

					_nodes[i].box.Min.y = glm::min(glm::min(_nodes[i].box.Min.y, oMin), oMax);
					_nodes[i].box.Max.y = glm::max(glm::max(_nodes[i].box.Max.y, oMin), oMax);
				}
			}

			ComputeHeights(GetRoot());
		}

	private:
		AABB BuildChildBox(uint index, const AABB& parentBox) const override
		{
			glm::vec3 signs;
			signs.x = (index & 1) ? 1.0f : -1.0f;
			signs.z = (index & 2) ? 1.0f : -1.0f;
			signs.y = 1.0f;

			glm::vec3 step = parentBox.GetExtent() * 0.5f;
			glm::vec3 center = parentBox.GetCenter() + step * signs;

			AABB box = AABB(center - step, center + step);
			box.Min.y = parentBox.Min.y;
			box.Max.y = parentBox.Max.y;
			return box;
		}

		void CollectHeights(Node* node, float& currMin, float& currMax)
		{
			if (node->objects.size())
			{
				currMin = glm::min(glm::min(node->box.Min.y, currMin), node->box.Max.y);
				currMax = glm::max(glm::max(node->box.Min.y, currMax), node->box.Max.y);
			}

			if (node->children)
			{
				for (uint i = 0; i < _childCount; i++)
					CollectHeights(&node->children[i], currMin, currMax);
			}
		}

		void ComputeHeights(Node* node)
		{
			if (node->children)
			{
				for (uint i = 0; i < _childCount; i++)
					CollectHeights(&node->children[i], node->box.Min.y, node->box.Max.y);
			}
			//else
			//{
			//	float minH = node->box.Min.y;
			//	float maxH = node->box.Max.y;
			//	Node* parent = node->parent;
			//	while (parent)
			//	{
			//		parent->box.Min.y = glm::min(parent->box.Min.y, minH);
			//		parent->box.Max.y = glm::max(parent->box.Max.y, maxH);
			//		minH = parent->box.Min.y;
			//		maxH = parent->box.Max.y;
			//		parent = parent->parent;
			//	}
			//}
		}
	};

}