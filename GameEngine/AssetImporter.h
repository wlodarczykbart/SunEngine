#pragma once

#include "Types.h"
#include "3DImporter.h"

namespace SunEngine
{
	class AssetNode;
	class Asset;
	class Mesh;
	class Material;
	class Texture2D;
	struct AABB;

	class AssetImporter
	{
	public:
		struct Options
		{
			bool CombineMaterials;
			uint MaxTextureSize;

			static const Options Default;
		};

		AssetImporter();
		~AssetImporter();

		bool Import(const String& filename, const Options& options = Options::Default);
		Asset* GetAsset() const { return _asset; }
	private:
		enum MaterialUsageFlags
		{
			MUF_NONE,
			MUF_MESH,
			MUF_SKINNED_MESH,
		};

		bool ChooseMaterial(void* iMesh, Material*& pOutMtl);

		Options _options;
		Asset* _asset;
		Map<void*, Mesh*> _meshFixup;
		Map<void*, Pair<Material*, StrMap<Texture2D*>>> _materialFixup;
		Map<void*, AssetNode*> _nodeFixup;
		Vector<AssetNode*> _nodes;
		Map<Texture2D*, String> _textureLoadList;
	};

}