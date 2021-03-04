#pragma once

#include "Mesh.h"
#include "Material.h"
#include "Texture2D.h"
#include "Shader.h"
#include "Asset.h"
#include "Sampler.h"

#define MakeResDef(Type, Name) const String Type::Name = #Name;

namespace SunEngine
{
	namespace DefaultRes
	{
		class Texture
		{
		public:
			static const String Default;
			static const String White;
			static const String Black;
			static const String Zero;
			static const String Normal;
		};

		class Mesh
		{
		public:
			static const String Cube;
			static const String Sphere;
		};

		class Shader
		{
		public:
			static const String Standard;
			static const String BlinnPhong;
		};

		class Material
		{
		public:
			static const String StandardDefault;
			static const String BlinnPhongDefault;
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
		Shader* AddShader(const String& name);
		Mesh* AddMesh(const String& name);
		Material* AddMaterial(const String& name);
		Texture2D* AddTexture2D(const String& name);

		Asset* GetAsset(const String& name) const;
		Shader* GetShader(const String& name) const;
		Mesh* GetMesh(const String& name) const;
		Material* GetMaterial(const String& name) const;
		Texture2D* GetTexture2D(const String& name) const;
		Sampler* GetSampler(FilterMode filter, WrapMode wrap, AnisotropicMode anisotropy);

		Iter<Material> IterMaterials() const;
		Iter<Texture2D> IterTextures2D() const;

		bool Remove(Asset* pAsset);

	private:
		template<typename T>
		T* AddResourceToMap(StrMap<UniquePtr<T>>& map, const String& name);

		ResourceMgr();
		~ResourceMgr();

		StrMap<UniquePtr<Asset>> _assets;
		StrMap<UniquePtr<Shader>> _shaders;
		StrMap<UniquePtr<Mesh>> _meshes;
		StrMap<UniquePtr<Material>> _materials;
		StrMap<UniquePtr<Texture2D>> _textures2D;

		StrMap<UniquePtr<Sampler>> _samplers;

	};
}