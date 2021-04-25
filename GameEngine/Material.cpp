#include <functional>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/hash.hpp"
#undef GLM_ENABLE_EXPERIMENTAL
#endif

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

	Material::Material()
	{
		_shader = 0;
		_depthVariantHash = 0;
		_depthVariantDefineHash = 0;
	}

	Material::~Material()
	{

	}

	void Material::SetShader(Shader* pShader)
	{
		String strVariant;

		switch (EngineInfo::GetRenderer().RenderMode())
		{
		case EngineInfo::Renderer::Forward:
			strVariant = Shader::Default;
			break;
		case EngineInfo::Renderer::Deferred:
			strVariant = Shader::GBuffer;
			break;
		default:
			strVariant = Shader::Default;
			break;
		}

		if(!pShader->GetVariant(strVariant))
			strVariant = Shader::Default;

		SetShader(pShader, strVariant);
	}

	void Material::SetShader(Shader* pShader, const String& variant)
	{
		_shader = pShader;
		_variant = variant;

		_depthVariantProps.clear();
		if (variant != Shader::Depth)
		{
			pShader->GetVariantProps(Shader::Depth, _depthVariantProps);

			String strHash;
			Vector<String> defines;
			pShader->GetVariantDefines(Shader::Depth, defines);
			for (const String& def : defines)
				strHash += def;
			_depthVariantDefineHash = std::hash<String>()(strHash);
			UpdateDepthVariantHash();
		}
	}

	bool Material::SetMaterialVar(const String& name, const void* pData, const uint size)
	{
		auto found = _mtlVariables.find(name);
		if (found != _mtlVariables.end() && (*found).second.size <= size)
		{
			_memBuffer.SetData(pData, size, (*found).second.offset);
			if (!_mtlBuffer.Update(_memBuffer.GetData()))
				return false;

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

		BaseShader* pVariant = _shader->GetVariant(_variant);

		ShaderBindings::CreateInfo info = {};
		info.pShader = pVariant;
		info.type = SBT_MATERIAL;

		if (!_gpuObject.Create(info))
			return false;

		_mtlVariables.clear();
		_mtlTextures2D.clear();
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
						if (res.dimension == SRD_TEXTURE_2D)
						{
							MaterialTextureData data = {};
							data.Res = res;
							_mtlTextures2D[res.name] = data;
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

	BaseShader* Material::GetShaderVariant() const
	{
		return _shader ? _shader->GetVariant(_variant) : 0;
	}

	bool Material::Write(StreamBase& stream)
	{
		if (!GPUResource::Write(stream))
			return false;

		if (!stream.Write(_shader)) return false;
		if (!stream.Write(_variant)) return false;
		if (!_memBuffer.Write(stream)) return false;
		if (!stream.WriteSimple(_mtlVariables)) return false;
		if (!stream.WriteSimple(_mtlTextures2D)) return false;
		if (!stream.WriteSimple(_mtlSamplers)) return false;

		return true;
	}

	bool Material::Read(StreamBase& stream)
	{
		if (!GPUResource::Read(stream))
			return false;

		if (!stream.Read(_shader)) return false;
		if (!stream.Read(_variant)) return false;
		if (!_memBuffer.Read(stream)) return false;
		if (!stream.ReadSimple(_mtlVariables)) return false;
		if (!stream.ReadSimple(_mtlTextures2D)) return false;
		if (!stream.ReadSimple(_mtlSamplers)) return false;

		return true;
	}

	bool Material::CreateDepthMaterial(Material* pEmptyMaterial) const
	{
		pEmptyMaterial->SetShader(_shader, Shader::Depth);
		if (!pEmptyMaterial->RegisterToGPU())
			return false;

		for (auto& prop : _depthVariantProps)
		{
			switch (prop.second.Type)
			{
			case SPT_TEXTURE2D:
				pEmptyMaterial->SetTexture2D(prop.first, prop.second.pTexture2D);
				break;
			case SPT_SAMPLER:
				pEmptyMaterial->SetSampler(prop.first, prop.second.pSampler);
				break;
			case SPT_FLOAT:
				pEmptyMaterial->SetMaterialVar(prop.first, prop.second.float1);
				break;
			case SPT_FLOAT2:
				pEmptyMaterial->SetMaterialVar(prop.first, prop.second.float2);
				break;
			case SPT_FLOAT3:
				pEmptyMaterial->SetMaterialVar(prop.first, prop.second.float3);
				break;
			case SPT_FLOAT4:
				pEmptyMaterial->SetMaterialVar(prop.first, prop.second.float4);
				break;
			default:
				break;
			}
		}

		return true;
	}

	void Material::UpdateDepthVariant(const String& name, const ShaderProp& prop)
	{
		auto found = _depthVariantProps.find(name);
		if (found != _depthVariantProps.end())
		{
			(*found).second = prop;
			UpdateDepthVariantHash();
		}
	}

	void Material::UpdateDepthVariantHash()
	{
		_depthVariantHash = _depthVariantDefineHash;
		for (auto& prop : _depthVariantProps)
		{
			usize keyHash = std::hash<String>()(prop.first);
			usize valHash = 0;
			switch (prop.second.Type)
			{
			case SPT_TEXTURE2D:
				valHash = (usize)prop.second.pTexture2D;
				break;
			case SPT_SAMPLER:
				valHash = (usize)prop.second.pSampler;
				break;
			case SPT_FLOAT:
				valHash = std::hash<float>()(prop.second.float1);
				break;
			case SPT_FLOAT2:
				valHash = std::hash<glm::vec2>()(prop.second.float2);
				break;
			case SPT_FLOAT3:
				valHash = std::hash<glm::vec3>()(prop.second.float3);
				break;
			case SPT_FLOAT4:
				valHash = std::hash<glm::vec4>()(prop.second.float4);
				break;
			default:
				break;
			}

			_depthVariantHash += keyHash << 2;
			_depthVariantHash += valHash >> 2;
		}
	}
}