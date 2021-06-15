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

		enum TextureChannel
		{
			TC_RED,
			TC_GREEN,
			TC_BLUE,
			TC_ALPHA,
		};

		struct TextureLoadTask
		{
			typedef void(*TransformPixelFunc)(Pixel& p);

			TextureLoadTask(Texture2D* pTexture, TextureChannel r, TextureChannel g, TextureChannel b, TextureChannel a, bool compress, bool srgb, TransformPixelFunc func = 0)
			{
				Texture = pTexture;
				R = r;
				G = g;
				B = b;
				A = a;
				Compress = compress;
				SRGB = srgb;
				TransformFunc = func;

			}

			Texture2D* Texture;
			TextureChannel R;
			TextureChannel G;
			TextureChannel B;
			TextureChannel A;
			bool Compress;
			bool SRGB;
			TransformPixelFunc TransformFunc;
		};

		bool ChooseMaterial(void* iMesh, Material*& pOutMtl);

		Options _options;
		String _path;
		Asset* _asset;
		Map<void*, Mesh*> _meshFixup;
		Map<void*, Pair<Material*, StrMap<Texture2D*>>> _materialFixup;
		Map<void*, AssetNode*> _nodeFixup;
		Vector<AssetNode*> _nodes;
		Map<Texture2D*, String> _textureLoadList;
		Map<Texture2D* , Vector<TextureLoadTask>> _textureLoadTasks;
		Map<Material*, Vector<Pair<String, Texture2D*>>> _materialMapping;
	};

}