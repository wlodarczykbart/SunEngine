#include "ResourceMgr.h"
#include "StringUtil.h"
#include "FilePathMgr.h"
#include "ShaderCompiler.h"
#include "FileBase.h"
#include "Shader.h"

namespace SunEngine
{
#define VARIANT_ENTRY(e) { #e, ShaderVariant::e } 

	static const Map<String, uint> VariantStringToBitMap =
	{
		VARIANT_ENTRY(GBUFFER),
		VARIANT_ENTRY(DEPTH),
		VARIANT_ENTRY(ALPHA_TEST),
		VARIANT_ENTRY(SKINNED),
		VARIANT_ENTRY(ONE_Z),
		VARIANT_ENTRY(SIMPLE_SHADING),
		VARIANT_ENTRY(KERNEL_3X3),
		VARIANT_ENTRY(KERNEL_5X5),
		VARIANT_ENTRY(KERNEL_7X7),
		VARIANT_ENTRY(KERNEL_9X9),
	};

	Shader::Shader()
	{

	}

	Shader::~Shader()
	{
	}

	bool Shader::Compile(const String& vertexSource, const String& pixelSource, String* pErrStr, Vector<String>* pDefines)
	{
		ConfigFile config;
		ConfigSection* section = config.AddSection("Shader");
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
		_name = GetFileNameNoExt(config.GetFilename());

		const ConfigSection* pShaderSection = config.GetSection("Shader");
		if (pShaderSection == 0)
			return false;

		//shaders require a vertex shader at minimum
		String vs = pShaderSection->GetString("vs");
		if (vs.empty())
			return false;
		String ps = pShaderSection->GetString("ps");
		String gs = pShaderSection->GetString("gs");

		String sourceDir = EngineInfo::GetPaths().ShaderSourceDir();
		bool isText = pShaderSection->GetInt("isText", 0) == 1;
		if (!isText)
		{
			Vector<String*> inputFiles = { &vs, &ps, &gs };
			for (auto pFilename : inputFiles)
			{
				if (!pFilename->empty())
				{
					FileStream fr;
					String path = sourceDir + *pFilename;
					if (fr.OpenForRead(path.c_str()))
					{
						fr.ReadText(*pFilename);
						fr.Close();
					}
					else
					{
						*pErrStr = StrFormat("Failed to open %s", path.c_str());
						return false;
					}
				}
			}
		}

		ShaderCompiler baseCompiler;
		if(pDefines)
			baseCompiler.SetDefines(*pDefines);
		baseCompiler.SetVertexShaderSource(vs);
		baseCompiler.SetPixelShaderSource(ps);

		if (!baseCompiler.Compile(_name))
		{
			if (pErrStr)
				*pErrStr = baseCompiler.GetLastError();
			return false;
		}

		ShaderVariantData* pBase = new ShaderVariantData();
		_variants[0] = UniquePtr<ShaderVariantData>(pBase);
		if (!pBase->shader.Create(baseCompiler.GetCreateInfo()))
		{
			if (pErrStr)
				*pErrStr = pBase->shader.GetErrStr();
			return false;
		}

		const ConfigSection* pVariantSection = config.GetSection("Variants");
		if (pVariantSection)
		{
			for (auto iter = pVariantSection->Begin(); iter != pVariantSection->End(); ++iter)
			{
				String variantString = (*iter).first;
				String variantShaders = (*iter).second;

				Vector<String> variantDefines;
				StrSplit(variantString, variantDefines, ',');
				Vector<String> variantOnlyDefines = variantDefines;

				if (pDefines)
					variantDefines.insert(variantDefines.end(), pDefines->begin(), pDefines->end());

				ShaderCompiler variantCompiler;
				variantCompiler.SetDefines(variantDefines);

				if (variantShaders.empty())
				{
					variantCompiler.SetVertexShaderSource(vs);
					variantCompiler.SetPixelShaderSource(ps);
				}
				else
				{
					if (StrContains(variantShaders, "vs")) variantCompiler.SetVertexShaderSource(vs);
					if (StrContains(variantShaders, "ps")) variantCompiler.SetPixelShaderSource(ps);
				}

				String variantName = _name.size() ? _name + "_" + StrReplace(variantString, { ',' }, '_') : "";
				if (!variantCompiler.Compile(variantName))
				{
					if (pErrStr)
						*pErrStr = variantCompiler.GetLastError();
					return false;
				}


				uint64 variantMask = 0;
				for (uint i = 0; i < variantOnlyDefines.size(); i++)
				{
					auto found = VariantStringToBitMap.find(variantOnlyDefines[i]);
					if (found != VariantStringToBitMap.end())
					{
						variantMask |= (*found).second;
					}
				}

				if (variantMask == 0)
				{
					if (pErrStr)
						*pErrStr = StrFormat("Shader variant mask %s consits of only unsupported variant defines", variantString.c_str());
					return false;
				}

				ShaderVariantData* pVariant = new ShaderVariantData();
				_variants[variantMask] = UniquePtr<ShaderVariantData>(pVariant);
				if (!pVariant->shader.Create(variantCompiler.GetCreateInfo()))
				{
					if (pErrStr)
						*pErrStr = pVariant->shader.GetErrStr();
					return false;
				}
				pVariant->defines = variantOnlyDefines;
			}
		}

		_config = config;
		SetDefaults();
		return true;
	}

	void Shader::SetDefaults(Material* pMtl) const
	{
		if (pMtl->GetShader() != this)
			return;

		auto& defaults = _variants.at(0).get()->defaults;

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

	BaseShader* Shader::GetBase() const
	{
		return GetBaseVariant(0); //TODO: store this as class variable?
	}

	BaseShader* Shader::GetBaseVariant(uint64 variantMask) const
	{
		auto found = _variants.find(variantMask);
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

	bool Shader::GetVariantProps(uint64 variantMask, StrMap<ShaderProp>** props) const
	{
		auto found = _variants.find(variantMask);
		if (found != _variants.end())
		{
			*props = &(*found).second.get()->defaults;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Shader::ContainsVariants(uint64 variantMask) const
	{
		for (auto iter = _variants.begin(); iter != _variants.end(); ++iter)
		{
			if ((*iter).first & variantMask)
				return true;
		}
		return false;
	}

	//bool Shader::GetVariantDefines(uint64 variantMask, Vector<String>& defines) const
	//{
	//	auto found = _variants.find(variantMask);
	//	if (found != _variants.end())
	//	{
	//		defines = (*found).second.get()->defines;
	//		return true;
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

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

	void Shader::FillMatrices(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& sunDirection, CameraBufferData& camData)
	{
		glm::mat4 viewProj = proj * view;
		glm::mat4 invView = glm::inverse(view);
		glm::mat4 invProj = glm::inverse(proj);
		glm::mat4 invViewProj = glm::inverse(viewProj);
		glm::vec4 sunViewDir = glm::normalize(view * glm::vec4(sunDirection, 0.0f));
		camData.ViewMatrix.Set(&view);
		camData.ProjectionMatrix.Set(&proj);
		camData.ViewProjectionMatrix.Set(&viewProj);
		camData.InvViewMatrix.Set(&invView);
		camData.InvProjectionMatrix.Set(&invProj);
		camData.InvViewProjectionMatrix.Set(&invViewProj);
		camData.CameraData.row3.Set(&sunViewDir);
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
						if (res.dimension == SRD_TEXTURE2D)
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