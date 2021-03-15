#include "StringUtil.h"
#include "ResourceMgr.h"

namespace SunEngine
{
	namespace DefaultResource
	{
		DefineStaticStr(Texture, Default);
		DefineStaticStr(Texture, White);
		DefineStaticStr(Texture, Black);
		DefineStaticStr(Texture, Zero);
		DefineStaticStr(Texture, Normal);

		DefineStaticStr(Mesh, Cube);
		DefineStaticStr(Mesh, Sphere);

		DefineStaticStr(Shader, StandardMetallic);
		DefineStaticStr(Shader, StandardSpecular);
		DefineStaticStr(Shader, Gamma);

		DefineStaticStr(Material, StandardMetallic);
		DefineStaticStr(Material, StandardSpecular);
	}

	template<typename T>
	T* ResourceMgr::AddResourceToMap(StrMap<UniquePtr<T>>& map, const String& name)
	{
		uint counter = 0;
		String uniqueName = name;
		while (map.find(uniqueName) != map.end())
		{
			uniqueName = StrFormat("%s_%d", name.data(), ++counter);
		}

		T* pResource = new T();
		static_cast<Resource*>(pResource)->_name = uniqueName;
		map[uniqueName] = UniquePtr<T>(pResource);
		return pResource;
	}

	ResourceMgr& ResourceMgr::Get()
	{
		static ResourceMgr mgr;
		return mgr;
	}

	bool ResourceMgr::CreateDefaults()
	{
		Mesh* pCubeMesh = AddMesh(DefaultResource::Mesh::Cube);
		pCubeMesh->AllocateCube();
		if (!pCubeMesh->RegisterToGPU())
		{
			return false;
		}

		Mesh* pSphereMesh = AddMesh(DefaultResource::Mesh::Sphere);
		pSphereMesh->AllocateSphere();
		if (!pSphereMesh->RegisterToGPU())
		{
			return false;
		}

		Texture2D* pDefaultTexture = AddTexture2D(DefaultResource::Texture::Default);
		pDefaultTexture->Alloc(16, 16);

		for (uint i = 0; i < pDefaultTexture->GetWidth(); i++)
		{
			for (uint j = 0; j < pDefaultTexture->GetHeight(); j++)
			{
				pDefaultTexture->SetPixel(i, j, ((i + j) & 2) ? glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) : glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
		}

		if (!pDefaultTexture->RegisterToGPU())
		{
			return false;
		}

		StrMap<glm::vec4> SolidColorDefaults =
		{
			{ DefaultResource::Texture::White, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), },
			{ DefaultResource::Texture::Black, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), },
			{ DefaultResource::Texture::Zero, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), },
			{ DefaultResource::Texture::Normal, glm::vec4(0.5f, 0.5f, 1.0f, 0.0f), },
		};

		for (auto iter = SolidColorDefaults.begin(); iter != SolidColorDefaults.end(); ++iter)
		{
			Texture2D* pTexture = AddTexture2D((*iter).first);
			pTexture->Alloc(4, 4);
			pTexture->FillColor((*iter).second);

			if (!pTexture->RegisterToGPU())
			{
				return false;
			}
		}

		return true;
	}

	Asset* ResourceMgr::AddAsset(const String& name)
	{
		Asset* pRes = AddResourceToMap(_assets, name);
		return pRes;
	}

	Shader* ResourceMgr::AddShader(const String& name)
	{
		Shader* pRes = AddResourceToMap(_shaders, name);
		return pRes;
	}

	Mesh* ResourceMgr::AddMesh(const String& name)
	{
		Mesh* pRes = AddResourceToMap(_meshes, name);
		return pRes;
	}

	Material* ResourceMgr::AddMaterial(const String& name)
	{
		Material* pRes = AddResourceToMap(_materials, name);
		return pRes;
	}

	Texture2D* ResourceMgr::AddTexture2D(const String& name)
	{
		Texture2D* pRes = AddResourceToMap(_textures2D, name);
		return pRes;
	}

	Asset* ResourceMgr::GetAsset(const String& name) const
	{
		auto found = _assets.find(name);
		return found != _assets.end() ? (*found).second.get() : 0;
	}

	Shader* ResourceMgr::GetShader(const String& name) const
	{
		auto found = _shaders.find(name);
		return found != _shaders.end() ? (*found).second.get() : 0;
	}

	Mesh* ResourceMgr::GetMesh(const String& name) const
	{
		auto found = _meshes.find(name);
		return found != _meshes.end() ? (*found).second.get() : 0;
	}

	Material* ResourceMgr::GetMaterial(const String& name) const
	{
		auto found = _materials.find(name);
		return found != _materials.end() ? (*found).second.get() : 0;
	}

	Texture2D* ResourceMgr::GetTexture2D(const String& name) const
	{
		auto found = _textures2D.find(name);
		return found != _textures2D.end() ? (*found).second.get() : 0;
	}

	ResourceMgr::ResourceMgr()
	{
	}

	ResourceMgr::~ResourceMgr()
	{
	}


	Sampler* ResourceMgr::GetSampler(FilterMode filter, WrapMode wrap, AnisotropicMode anisotropy)
	{
		String strSampler = StrFormat("%d_%d_%d", filter, wrap, anisotropy);
		auto found = _samplers.find(strSampler);
		if (found != _samplers.end())
		{
			return (*found).second.get();
		}

		Sampler::CreateInfo info = {};
		info.settings.anisotropicMode = anisotropy;
		info.settings.filterMode = filter;
		info.settings.wrapMode = wrap;

		Sampler* pSampler = new Sampler();
		if (!pSampler->Create(info))
			return 0;

		_samplers[strSampler] = UniquePtr<Sampler>(pSampler);
		return pSampler;
	}


	ResourceMgr::Iter<Material> ResourceMgr::IterMaterials() const
	{
		Iter<Material> iter;
		iter._cur = _materials.begin();
		iter._end = _materials.end();
		return iter;
	}

	ResourceMgr::Iter<Texture2D> ResourceMgr::IterTextures2D() const
	{
		Iter<Texture2D> iter;
		iter._cur = _textures2D.begin();
		iter._end = _textures2D.end();
		return iter;
	}

	bool ResourceMgr::Remove(Asset* pAsset)
	{
		if (!pAsset)
			return false;

		auto found = _assets.find(pAsset->GetName());
		if (found == _assets.end() || (*found).second.get() != pAsset)
			return false;

		_assets.erase(found);
		return true;
	}
}