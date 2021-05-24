#include "Shader.h"
#include "Texture2D.h"
#include "ResourceMgr.h"
#include "FilePathMgr.h"
#include "Material.h"

namespace SunEngine
{
	DefineStaticStr(MaterialStrings, DiffuseMap);
	DefineStaticStr(MaterialStrings, NormalMap);
	DefineStaticStr(MaterialStrings, SpecularMap);
	DefineStaticStr(MaterialStrings, MetalMap);
	DefineStaticStr(MaterialStrings, AmbientOcclusionMap);
	DefineStaticStr(MaterialStrings, SmoothnessMap);
	DefineStaticStr(MaterialStrings, DiffuseColor);
	DefineStaticStr(MaterialStrings, SpecularColor);
	DefineStaticStr(MaterialStrings, Smoothness);
	DefineStaticStr(MaterialStrings, Sampler);
	DefineStaticStr(MaterialStrings, PositionMap);
	DefineStaticStr(MaterialStrings, DepthMap);
	DefineStaticStr(MaterialStrings, AlphaMap);

	Material::Material()
	{
		_shader = 0;
		//_depthVariantHash = 0;
		//_depthVariantDefineHash = 0;
	}

	Material::~Material()
	{

	}

	void Material::SetShader(Shader* pShader)
	{
		_shader = pShader;
		//if (variant != Shader::Depth)
		//{
		//	pShader->GetVariantProps(Shader::Depth, _depthVariantProps);

		//	String strHash;
		//	Vector<String> defines;
		//	pShader->GetVariantDefines(Shader::Depth, defines);
		//	for (const String& def : defines)
		//		strHash += def;
		//	_depthVariantDefineHash = std::hash<String>()(strHash);
		//	UpdateDepthVariantHash();
		//}
	}

	bool Material::SetMaterialVar(const String& name, const void* pData, uint size)
	{
		auto found = _mtlVariables.find(name);
		if (found != _mtlVariables.end() && size <= (*found).second.size)
		{
			_memBuffer.SetData(pData, glm::min(size, (*found).second.size), (*found).second.offset);
			if (!_mtlBuffer.Update(_memBuffer.GetData()))
				return false;

			return true;
		}
		else
		{
			return false;
		}
	}

	bool Material::GetMaterialVar(const String& name, void* pData, uint size) const
	{
		auto found = _mtlVariables.find(name);
		if (found != _mtlVariables.end() && size <= (*found).second.size)
		{
			memcpy(pData, _memBuffer.GetData((*found).second.offset), glm::min(size, (*found).second.size));
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Material::SetTexture2D(const String& name, Texture2D* pTexture)
	{
		auto found = _mtlTextures2D.find(name);
		if (found != _mtlTextures2D.end())
		{
			if (!_gpuObject.SetTexture(name, pTexture->GetGPUObject()))
				return false;

			(*found).second.pTexture = pTexture;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Material::SetTextureCube(const String& name, TextureCube* pTexture)
	{
		auto found = _mtlTextureCubes.find(name);
		if (found != _mtlTextureCubes.end())
		{
			if (!_gpuObject.SetTextureCube(name, pTexture->GetGPUObject()))
				return false;

			(*found).second.pTextureCube = pTexture;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Material::SetSampler(const String& name, Sampler* pSampler)
	{
		auto found = _mtlSamplers.find(name);
		if (found != _mtlSamplers.end())
		{
			if (!_gpuObject.SetSampler(name, pSampler))
				return false;

			return true;
		}
		else
		{
			return false;
		}
	}

	bool Material::RegisterToGPU()
	{
		if (!_shader)
			return false;

		if (!_gpuObject.Destroy())
			return false;

		BaseShader* pVariant = _shader->GetBase();

		ShaderBindings::CreateInfo info = {};
		info.pShader = pVariant;
		info.type = SBT_MATERIAL;

		if (!_gpuObject.Create(info))
			return false;

		_mtlVariables.clear();
		_mtlTextures2D.clear();
		_mtlTextureCubes.clear();
		_mtlSamplers.clear();

		if (_shader)
		{
			Vector<IShaderBuffer> buffInfos;
			Vector<IShaderResource> resInfos;
			pVariant->GetBufferInfos(buffInfos);
			pVariant->GetResourceInfos(resInfos);

			for (uint i = 0; i < buffInfos.size(); i++)
			{
				if (buffInfos[i].name == ShaderStrings::MaterialBufferName && buffInfos[i].bindType == SBT_MATERIAL)
				{
					IShaderBuffer& buff = buffInfos[i];

					UniformBuffer::CreateInfo buffCreateInfo = {};
					buffCreateInfo.isShared = false;
					buffCreateInfo.size = buff.size;
					if (!_mtlBuffer.Create(buffCreateInfo))
						return false;

					_memBuffer.SetSize(buff.size);
					if (!_gpuObject.SetUniformBuffer(buff.name, &_mtlBuffer))
						return false;

					for (uint j = 0; j < buff.numVariables; j++)
					{
						_mtlVariables[buff.variables[j].name] = buff.variables[j];
					}

					break;
				}
			}

			for (uint i = 0; i < resInfos.size(); i++)
			{
				if (resInfos[i].bindType == SBT_MATERIAL)
				{
					IShaderResource& res = resInfos[i];
					if (res.type == SRT_TEXTURE)
					{
						MaterialTextureData data = {};
						data.Res = res;
						switch (res.dimension)
						{
						case SRD_TEXTURE_2D:
							_mtlTextures2D[res.name] = data;
							break;
						case SRD_TEXTURE_CUBE:
							_mtlTextureCubes[res.name] = data;
							break;
						default:
							break;
						}
					}
					else if (res.type == SRT_SAMPLER)
					{
						MaterialSamplerData data = {};
						data.Res = res;
						_mtlSamplers[res.name] = data;
					}
				}
			}

			_shader->SetDefaults(this);
		}

		return true;
	}

	bool Material::Write(StreamBase& stream)
	{
		if (!GPUResource::Write(stream))
			return false;

		if (!stream.Write(_shader)) return false;
		if (!_memBuffer.Write(stream)) return false;
		if (!stream.WriteSimple(_mtlVariables)) return false;
		if (!stream.WriteSimple(_mtlTextures2D)) return false;
		if (!stream.WriteSimple(_mtlTextureCubes)) return false;
		if (!stream.WriteSimple(_mtlSamplers)) return false;

		return true;
	}

	bool Material::Read(StreamBase& stream)
	{
		if (!GPUResource::Read(stream))
			return false;

		if (!stream.Read(_shader)) return false;
		if (!_memBuffer.Read(stream)) return false;
		if (!stream.ReadSimple(_mtlVariables)) return false;
		if (!stream.ReadSimple(_mtlTextures2D)) return false;
		if (!stream.ReadSimple(_mtlTextureCubes)) return false;
		if (!stream.ReadSimple(_mtlSamplers)) return false;

		return true;
	}
}