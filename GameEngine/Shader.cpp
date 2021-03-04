#include "ResourceMgr.h"
#include "StringUtil.h"
#include "Shader.h"

namespace SunEngine
{
	MakeResDef(StandardShader, DiffuseMap);
	MakeResDef(StandardShader, NormalMap);
	MakeResDef(StandardShader, MetalMap);
	MakeResDef(StandardShader, AmbientOcclusionMap);
	MakeResDef(StandardShader, RoughnessMap);

	MakeResDef(BlinnPhongShader, DiffuseMap);
	MakeResDef(BlinnPhongShader, NormalMap);
	MakeResDef(BlinnPhongShader, SpecularMap);

	Shader::Shader()
	{
		_createInfo = {};
	}

	Shader::~Shader()
	{
	}

	bool Shader::LoadConfig(const String& path)
	{
		if (!_config.Load(path.c_str()))
			return false;

		SetDefaults();
		return true;
	}

	void Shader::SetCreateInfo(const BaseShader::CreateInfo& info)
	{
		_createInfo = info;
		SetDefaults();
	}

	bool Shader::RegisterToGPU()
	{
		if (!_gpuObject.Destroy())
			return false;

		if (!_gpuObject.Create(_createInfo))
			return false;

		return true;
	}

	void Shader::SetDefaults(Material* pMtl) const
	{
		if (pMtl->GetShader() != this)
			return;

		for (auto iter = _defaultValues.begin(); iter != _defaultValues.end(); ++iter)
		{
			const DefaultValue& dv = (*iter).second;
			switch (dv.Type)
			{
			case DV_TEXTURE2D:
				pMtl->SetTexture2D((*iter).first, dv.pTexture2D);
				break;
			case DV_SAMPLER:
				pMtl->SetSampler((*iter).first, dv.pSampler);
				break;
			case DV_FLOAT:
				pMtl->SetMaterialVar((*iter).first, dv.float1);
				break;
			case DV_FLOAT2:
				pMtl->SetMaterialVar((*iter).first, dv.float2);
				break;
			case DV_FLOAT3:
				pMtl->SetMaterialVar((*iter).first, dv.float3);
				break;
			case DV_FLOAT4:
				pMtl->SetMaterialVar((*iter).first, dv.float4);
				break;
			default:
				break;
			}
		}
	}

	void Shader::ParseSamplerAnisotropy(const String& str, FilterMode& fm, WrapMode& wm, AnisotropicMode& am) const
	{
		static const StrMap<FilterMode> FILTER_LOOKOUP =
		{
			{ "LINEAR", SE_FM_LINEAR },
			{ "NEAREST", SE_FM_NEAREST },
		};

		static const StrMap<WrapMode> WRAP_LOOKOUP =
		{
			{ "CLAMP", SE_WM_CLAMP_TO_EDGE },
			{ "REPEAT", SE_WM_REPEAT },
		};

		static const StrMap<AnisotropicMode> ANISOTROPY_LOOKOUP =
		{
			{ "1", SE_AM_OFF, },
			{ "2", SE_AM_2, },
			{ "4", SE_AM_4, },
			{ "8", SE_AM_8, },
			{ "16", SE_AM_16, },
		};

		Vector<String> parts;
		StrSplit(StrRemove(str, ' '), parts, ',');

		static const String FILTER_KEY = "Filter=";
		static const String WRAP_KEY = "Wrap=";
		static const String ANISOTROPY_KEY = "Samples=";

		for (uint i = 0; i < parts.size(); i++)
		{
			String keyValue = parts[i];
			if (StrStartsWith(keyValue, FILTER_KEY))
			{
				auto found = FILTER_LOOKOUP.find(parts[i].substr(FILTER_KEY.length()));
				fm = found == FILTER_LOOKOUP.end() ? SE_FM_LINEAR : (*found).second;
			}
			else if (StrStartsWith(keyValue, WRAP_KEY))
			{
				auto found = WRAP_LOOKOUP.find(parts[i].substr(WRAP_KEY.length()));
				wm = found == WRAP_LOOKOUP.end() ? SE_WM_REPEAT : (*found).second;
			}
			else if (StrStartsWith(keyValue, ANISOTROPY_KEY))
			{
				auto found = ANISOTROPY_LOOKOUP.find(parts[i].substr(ANISOTROPY_KEY.length()));
				am = found == ANISOTROPY_LOOKOUP.end() ? SE_AM_OFF : (*found).second;
			}
		}
	}

	void Shader::ParseFloats(const String& str, uint maxComponents, float* pData) const
	{
		Vector<String> parts;
		StrSplit(StrRemove(str, ' '), parts, ',');
		for (uint i = 0; i < parts.size() && i < maxComponents; i++)
		{
			pData[i] = StrToFloat(parts[i]);
		}
	}

	void Shader::SetDefaults()
	{
		ResourceMgr& resMgr = ResourceMgr::Get();

		_defaultValues.clear();
		for (auto iter = _createInfo.ResMap.begin(); iter != _createInfo.ResMap.end(); ++iter)
		{
			const IShaderResource& res = (*iter).second;
			if (res.bindType == SBT_MATERIAL)
			{

				if (res.type == SRT_TEXTURE)
				{
					if (res.dimension == SRD_TEXTURE_2D)
					{
						_defaultValues[res.name].Type = DV_TEXTURE2D;
						_defaultValues[res.name].pTexture2D = resMgr.GetTexture2D("Black");
					}
				}
				else if (res.type == SRT_SAMPLER)
				{
					_defaultValues[res.name].Type = DV_SAMPLER;
					_defaultValues[res.name].pSampler = resMgr.GetSampler(SE_FM_LINEAR, SE_WM_REPEAT, SE_AM_OFF);
				}
			}
		}

		for (auto iter = _createInfo.BuffMap.begin(); iter != _createInfo.BuffMap.end(); ++iter)
		{
			const IShaderBuffer& buff = (*iter).second;
			if (buff.bindType == SBT_MATERIAL)
			{
				for (uint i = 0; i < buff.Variables.size(); i++)
				{
					const ShaderBufferVariable& var = buff.Variables[i];
					switch (var.Type)
					{
					case SDT_FLOAT:
						_defaultValues[var.Name].Type = DV_FLOAT;
						_defaultValues[var.Name].float1 = 0.0f;
						break;
					case SDT_FLOAT2:
						_defaultValues[var.Name].Type = DV_FLOAT2;
						_defaultValues[var.Name].float2 = glm::vec2(0.0f);
						break;
					case SDT_FLOAT3:
						_defaultValues[var.Name].Type = DV_FLOAT3;
						_defaultValues[var.Name].float3 = glm::vec3(0.0f);
						break;
					case SDT_FLOAT4:
						_defaultValues[var.Name].Type = DV_FLOAT4;
						_defaultValues[var.Name].float4 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
						break;
					default:
						break;
					}
				}
			}
		}

		ConfigSection* pDefaults = _config.GetSection("Defaults");
		if (pDefaults)
		{
			for (auto iter = pDefaults->Begin(); iter != pDefaults->End(); ++iter)
			{
				auto found = _defaultValues.find((*iter).first);
				if (found != _defaultValues.end())
				{
					DefaultValue& dv = (*found).second;
					switch (dv.Type)
					{
					case DV_TEXTURE2D:
						dv.pTexture2D = resMgr.GetTexture2D((*iter).second);
						break;
					case DV_SAMPLER:
					{
						FilterMode fm = dv.pSampler->GetFilter();
						WrapMode wm = dv.pSampler->GetWrap();
						AnisotropicMode am = dv.pSampler->GetAnisotropy();
						ParseSamplerAnisotropy((*iter).second, fm, wm, am);
						dv.pSampler = resMgr.GetSampler(fm, wm, am);
					}
						break;
					case DV_FLOAT:
						ParseFloats((*iter).second, 1, &dv.float1);
						break;
					case DV_FLOAT2:
						ParseFloats((*iter).second, 2, &dv.float2.x);
						break;
					case DV_FLOAT3:
						ParseFloats((*iter).second, 3, &dv.float3.x);
						break;
					case DV_FLOAT4:
						ParseFloats((*iter).second, 4, &dv.float4.x);
						break;
					default:
						break;
					}
				}
			}
		}

	}
}