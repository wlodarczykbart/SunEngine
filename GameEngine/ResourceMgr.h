#pragma once

#include "Mesh.h"
#include "Material.h"
#include "Texture2DArray.h"
#include "TextureCube.h"
#include "Shader.h"
#include "Asset.h"
#include "Sampler.h"

#define DefineStaticStr(Type, Name) const String Type::Name = #Name;

namespace SunEngine
{
	namespace DefaultResource
	{
		class Texture
		{
		public:
			static const String Default;
			static const String White;
			static const String Black;
			static const String Zero;
			static const String Normal;
			static const String Red;
		};

		class Mesh
		{
		public:
			static const String Cube;
			static const String Sphere;
			static const String Plane;
			static const String Quad;
		};

		class Material
		{
		public:
			static const String StandardMetallic;
			static const String StandardSpecular;
		};
	}

	class ResourceMgr
	{
	public:
		template<class T>
		class Iter
		{
		public:
			T* operator *() const { return (*_cur).second.get(); };
			T* operator ++() { ++_cur; return End() ? 0 : (*_cur).second.get(); }
			bool End() const { return _cur == _end; }

		private:
			friend class ResourceMgr;

			typename StrMap<UniquePtr<T>>::const_iterator _cur;
			typename StrMap<UniquePtr<T>>::const_iterator _end;
		};

		ResourceMgr(const ResourceMgr&) = delete;
		ResourceMgr& operator = (const ResourceMgr&) = delete;

		static ResourceMgr& Get();

		bool CreateDefaults();

		Asset* AddAsset(const String& name);
		Mesh* AddMesh(const String& name);
		Material* AddMaterial(const String& name);
		Texture2D* AddTexture2D(const String& name);
		TextureCube* AddTextureCube(const String& name);
		Texture2DArray* AddTexture2DArray(const String& name);

		Asset* GetAsset(const String& name) const;
		Mesh* GetMesh(const String& name) const;
		Material* GetMaterial(const String& name) const;
		Texture2D* GetTexture2D(const String& name) const;
		TextureCube* GetTextureCube(const String& name) const;
		Texture2DArray* GetTexture2DArray(const String& name) const;

		Iter<Material> IterMaterials() const;
		Iter<Texture2D> IterTextures2D() const;

		Material* Clone(Material* pSrc);

		bool Remove(Asset* pRes);
		bool Remove(Material* pRes);
		bool Remove(Texture2D* pRes);

		Sampler* GetSampler(FilterMode filter, WrapMode wrap);
		Sampler* GetSampler(FilterMode filter, WrapMode wrap, AnisotropicMode anisotropy);
		Sampler* GetSampler(FilterMode filter, WrapMode wrap, BorderColor border);
		Sampler* GetSampler(FilterMode filter, WrapMode wrap, AnisotropicMode anisotropy, BorderColor border);

		bool IsDefaultTexture2D(Texture2D* pTexture) const;
	private:
		template<typename T>
		T* AddResourceToMap(StrMap<UniquePtr<T>>& map, const String& name);

		template<typename T>
		bool RemoveResourceFromMap(StrMap<UniquePtr<T>>& map, T* resource);

		bool CloneResource(Resource* pDst, Resource* pSrc) const;

		ResourceMgr();
		~ResourceMgr();

		StrMap<UniquePtr<Asset>> _assets;
		StrMap<UniquePtr<Mesh>> _meshes;
		StrMap<UniquePtr<Material>> _materials;
		StrMap<UniquePtr<Texture2D>> _textures2D;
		StrMap<UniquePtr<TextureCube>> _textureCubes;
		StrMap<UniquePtr<Texture2DArray>> _texture2DArrays;

		StrMap<UniquePtr<Sampler>> _samplers;

		Vector<Texture2D*> _defaultTextures2D;

	};
}