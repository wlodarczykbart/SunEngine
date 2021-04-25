#include "ResourceMgr.h"
#include "StringUtil.h"
#include "FilePathMgr.h"
#include "ShaderCompiler.h"
#include "FileBase.h"
#include "Shader.h"

namespace SunEngine
{
	const String Shader::Default = "DefaultPass";
	const String Shader::GBuffer = "GBufferPass";
	const String Shader::Depth = "DepthPass";

	Shader::Shader()
	{

	}

	Shader::~Shader()
	{
	}

	bool Shader::Compile(const String& vertexSource, const String& pixelSource, String* pErrStr, Vector<String>* pDefines)
	{
		ConfigFile config;
		ConfigSection* section = config.AddSection(Default);
		section->SetString("vs", vertexSource.c_str());
		section->SetString("ps", pixelSource.c_str());
		section->SetInt("isText", 1);
		return Compile(config, pErrStr, pDefines);
	}

	bool Shader::Compile(const String& path, String* pErrStr, Vector<String>* pDefines)
	{
		ConfigFile config;
		if (config.Load(path))
			return Compile(config, pErrStr, pDefines);

		if (pErrStr)
			*pErrStr = "Failed to open " + path;
		return false;
	}

	bool Shader::Compile(const ConfigFile& config, String* pErrStr, Vector<String>* pDefines)
	{
		LinkedList<ConfigFile> configList;
		HashSet<String> configMap;

		configList.push_back(config);
		configMap.insert(config.GetFilename());
		CollectConfigFiles(configList, configMap);

		_name = GetFileNameNoExt(config.GetFilename());

		_config.Clear();
		while (configList.size())
		{
			ConfigFile& src = configList.back();
			for (auto iter = src.Begin(); iter != src.End(); ++iter)
			{
				auto& srcSection = (*iter).second;
				auto dstSection = _config.AddSection((*iter).first);
				for (auto subIter = srcSection.Begin(); subIter != srcSection.End(); ++subIter)
				{
					dstSection->SetString((*subIter).first, (*subIter).second.c_str());
				}
			}
			configList.pop_back();
		}

		Vector<ConfigSection*> variantSections;
		variantSections.push_back(_config.GetSection(Default));
		variantSections.push_back(_config.GetSection(GBuffer));
		variantSections.push_back(_config.GetSection(Depth));

		String shaderName = GetFileNameNoExt(config.GetFilename());

		String sourceDir = EngineInfo::GetPaths().ShaderSourceDir();

		for (ConfigSection* section : variantSections)
		{
			if (!section)
				continue;
			
			bool isText = section->GetInt("isText", 0) == 1;

			ShaderCompiler compiler;
			compiler.SetAuxiliaryDir(sourceDir);

			Vector<String> defines;
			StrSplit(section->GetString("defines"), defines, ',');
			Vector<String> variantOnlyDefines = defines;

			if (pDefines)
				defines.insert(defines.end(), pDefines->begin(), pDefines->end());
		
			compiler.SetDefines(defines);

			if (isText)
			{
				compiler.SetVertexShaderSource(section->GetString("vs"));
				compiler.SetPixelShaderSource(section->GetString("ps"));
			}
			else
			{
				FileStream fr;
				String text;

				if (fr.OpenForRead((sourceDir + section->GetString("vs")).c_str()))
				{
					if (fr.ReadText(text))
						compiler.SetVertexShaderSource(text);
					fr.Close();
				}

				if (fr.OpenForRead((sourceDir + section->GetString("ps")).c_str()))
				{
					if (fr.ReadText(text))
						compiler.SetPixelShaderSource(text);
					fr.Close();
				}
			}

			String variantName = shaderName.size() ? shaderName + "_" + section->GetName() : "";
			if (!compiler.Compile(variantName))
			{
				if(pErrStr) 
					*pErrStr = compiler.GetLastError();
				return false;
			}

			ShaderVariant* pVariant = new ShaderVariant();
			_variants[section->GetName()] = UniquePtr<ShaderVariant>(pVariant);
			if (!pVariant->shader.Create(compiler.GetCreateInfo()))
			{
				if (pErrStr)
					*pErrStr = pVariant->shader.GetErrStr();
				return false;
			}

			pVariant->defines = variantOnlyDefines;
		}

		SetDefaults();
		return true;
	}

	void Shader::SetDefaults(Material* pMtl) const
	{
		if (pMtl->GetShader() != this)
			return;

		auto found = _variants.find(pMtl->GetVariant());
		if (found == _variants.end())
			return;

		auto& defaults = (*found).second.get()->defaults;

		for (auto iter = defaults.begin(); iter != defaults.end(); ++iter)
		{
			const ShaderProp& dv = (*iter).second;
			switch (dv.Type)
			{
			case SPT_TEXTURE2D:
				pMtl->SetTexture2D((*iter).first, dv.pTexture2D);
				break;
			case SPT_SAMPLER:
				pMtl->SetSampler((*iter).first, dv.pSampler);
				break;
			case SPT_FLOAT:
				pMtl->SetMaterialVar((*iter).first, dv.float1);
				break;
			case SPT_FLOAT2:
				pMtl->SetMaterialVar((*iter).first, dv.float2);
				break;
			case SPT_FLOAT3:
				pMtl->SetMaterialVar((*iter).first, dv.float3);
				break;
			case SPT_FLOAT4:
				pMtl->SetMaterialVar((*iter).first, dv.float4);
				break;
			default:
				break;
			}
		}
	}

	BaseShader* Shader::GetDefault() const
	{
		return GetVariant(Default); //TODO: store this as class variable?
	}

	BaseShader* Shader::GetVariant(const String& variant) const
	{
		auto found = _variants.find(variant);
		return found != _variants.end() ? &(*found).second.get()->shader : 0;
	}

	bool Shader::GetConfigSection(const String& name, ConfigSection& section) const
	{
		auto found = _config.GetSection(name);
		if (found)
		{
			section = *found;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Shader::GetVariantProps(const String& name, StrMap<ShaderProp>& props) const
	{
		auto found = _variants.find(name);
		if (found != _variants.end())
		{
			props = (*found).second.get()->defaults;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Shader::GetVariantDefines(const String& name, Vector<String>& defines) const
	{
		auto found = _variants.find(name);
		if (found != _variants.end())
		{
			defines = (*found).second.get()->defines;
			return true;
		}
		else
		{
			return false;
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

	void Shader::CollectConfigFiles(LinkedList<ConfigFile>& configList, HashSet<String>& configMap)
	{
		ConfigFile& curr = configList.back();
		ConfigSection* input = curr.GetSection("Input");
		String strBase = input ? input->GetString("base") : "";
		if (strBase.length() && configMap.count(strBase) == 0)
		{
			ConfigFile base;
			String currDir = GetDirectory(curr.GetFilename()) + "/";
			if (base.Load(currDir + strBase))
			{
				configList.push_back(base);
				configMap.insert(base.GetFilename());
				CollectConfigFiles(configList, configMap);
			}
		}
	}

	void Shader::SetDefaults()
	{
		ResourceMgr& resMgr = ResourceMgr::Get();
		ConfigSection* pDefaults = _config.GetSection("Defaults");

		for (auto& variant : _variants)
		{
			auto pVariant = variant.second.get();

			pVariant->defaults.clear();

			Vector<IShaderBuffer> buffers;
			pVariant->shader.GetBufferInfos(buffers);

			Vector<IShaderResource> resources;
			pVariant->shader.GetResourceInfos(resources);

			for (auto iter = resources.begin(); iter != resources.end(); ++iter)
			{
				const IShaderResource& res = (*iter);
				if (res.bindType == SBT_MATERIAL)
				{

					if (res.type == SRT_TEXTURE)
					{
						if (res.dimension == SRD_TEXTURE_2D)
						{
							pVariant->defaults[res.name].Type = SPT_TEXTURE2D;
							pVariant->defaults[res.name].pTexture2D = resMgr.GetTexture2D("Black");
						}
					}
					else if (res.type == SRT_SAMPLER)
					{
						pVariant->defaults[res.name].Type = SPT_SAMPLER;
						pVariant->defaults[res.name].pSampler = resMgr.GetSampler(SE_FM_LINEAR, SE_WM_REPEAT, SE_AM_OFF);
					}
				}
			}

			for (auto iter = buffers.begin(); iter != buffers.end(); ++iter)
			{
				const IShaderBuffer& buff = (*iter);
				if (buff.bindType == SBT_MATERIAL)
				{
					for (uint i = 0; i < buff.numVariables; i++)
					{
						const ShaderBufferVariable& var = buff.variables[i];
						switch (var.type)
						{
						case SDT_FLOAT:
							pVariant->defaults[var.name].Type = SPT_FLOAT;
							pVariant->defaults[var.name].float1 = 0.0f;
							break;
						case SDT_FLOAT2:
							pVariant->defaults[var.name].Type = SPT_FLOAT2;
							pVariant->defaults[var.name].float2 = glm::vec2(0.0f);
							break;
						case SDT_FLOAT3:
							pVariant->defaults[var.name].Type = SPT_FLOAT3;
							pVariant->defaults[var.name].float3 = glm::vec3(0.0f);
							break;
						case SDT_FLOAT4:
							pVariant->defaults[var.name].Type = SPT_FLOAT4;
							pVariant->defaults[var.name].float4 = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
							break;
						default:
							break;
						}
					}
				}
			}

			if (pDefaults)
			{
				for (auto iter = pDefaults->Begin(); iter != pDefaults->End(); ++iter)
				{
					auto found = pVariant->defaults.find((*iter).first);
					if (found != pVariant->defaults.end())
					{
						ShaderProp& dv = (*found).second;
						switch (dv.Type)
						{
						case SPT_TEXTURE2D:
							dv.pTexture2D = resMgr.GetTexture2D((*iter).second);
							break;
						case SPT_SAMPLER:
						{
							FilterMode fm = dv.pSampler->GetFilter();
							WrapMode wm = dv.pSampler->GetWrap();
							AnisotropicMode am = dv.pSampler->GetAnisotropy();
							ParseSamplerAnisotropy((*iter).second, fm, wm, am);
							dv.pSampler = resMgr.GetSampler(fm, wm, am);
						}
						break;
						case SPT_FLOAT:
							ParseFloats((*iter).second, 1, &dv.float1);
							break;
						case SPT_FLOAT2:
							ParseFloats((*iter).second, 2, &dv.float2.x);
							break;
						case SPT_FLOAT3:
							ParseFloats((*iter).second, 3, &dv.float3.x);
							break;
						case SPT_FLOAT4:
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
}