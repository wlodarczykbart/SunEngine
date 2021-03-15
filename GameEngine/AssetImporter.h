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

			static const Options Default;
		};

		AssetImporter();
		~AssetImporter();

		bool Import(const String& filename, const Options& options = Options::Default);
		Asset* GetAsset() const { return _asset; }
	private:
		bool PickMaterialShader(void* iMaterial, String& shader, StrMap<Texture2D*>& textures);

		Asset* _asset;
		Map<void*, Mesh*> _meshFixup;
		Map<void*, Material*> _materialFixup;
		Map<void*, AssetNode*> _nodeFixup;
		Vector<AssetNode*> _nodes;
	};

}