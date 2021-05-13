#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream

#include "StringUtil.h"
#include "FilePathMgr.h"
#include "ResourceMgr.h"
#include "ShaderMgr.h"

namespace SunEngine
{
#define _RegisterField(type, field) { type obj; memset(&obj, 0x1, sizeof(obj)); std::stringstream ss; ss << obj.field; Register##type##Field(#field, (usize)(&obj.field) - (usize)&obj, sizeof(obj.field), StrContains(ss.str(), ".")); }

	DefineStaticStr(DefaultShaders, Metallic)
	DefineStaticStr(DefaultShaders, Specular)
	DefineStaticStr(DefaultShaders, SkinnedMetallic)
	DefineStaticStr(DefaultShaders, SkinnedSpecular)
	DefineStaticStr(DefaultShaders, ToneMap)
	DefineStaticStr(DefaultShaders, Deferred)
	DefineStaticStr(DefaultShaders, ScreenSpaceReflection)
	DefineStaticStr(DefaultShaders, SceneCopy)
	DefineStaticStr(DefaultShaders, FXAA)
	DefineStaticStr(DefaultShaders, TextureCopy)

	DefineStaticStr(DefaultPipelines, ShadowDepth)

	ShaderMgr& ShaderMgr::Get()
	{
		static ShaderMgr mgr;
		return mgr;
	}

	Shader* ShaderMgr::GetShader(const String& name) const
	{
		auto found = _shaders.find(name);
		return found != _shaders.end() ? (*found).second.get() : 0;
	}

	bool ShaderMgr::LoadShaders(String& errMsg)
	{
		String path = EngineInfo::GetPaths().ShaderListFile();
		ConfigFile config;
		if (!config.Load(path))
			return false;
		
		String shaderDir = EngineInfo::GetPaths().ShaderSourceDir();

		//Make sure these line up with what is in the shaders...
		Vector<String> defines;
		defines.push_back(StrFormat("MAX_SKINNED_BONES %d", EngineInfo::GetRenderer().MaxSkinnedBoneMatrices()));
		defines.push_back(StrFormat("MAX_TEXTURE_TRANSFORMS %d", EngineInfo::GetRenderer().MaxTextureTransforms()));
		defines.push_back(StrFormat("MAX_SHADOW_CASCADE_SPLITS %d", EngineInfo::GetRenderer().MaxShadowCascadeSplits()));

		ConfigSection* pList = config.GetSection("Shaders");
		for (auto iter = pList->Begin(); iter != pList->End(); ++iter)
		{
			String shaderName = GetFileNameNoExt((*iter).first);
			if (_shaders.find(shaderName) != _shaders.end())
				continue;

			String shaderConfig = shaderDir + (*iter).first;
			Shader* pShader = new Shader();
			_shaders[shaderName] = UniquePtr<Shader>(pShader);

			if (!pShader->Compile(shaderConfig, &errMsg, &defines))
			{
				errMsg = shaderConfig + "\n" + errMsg;
				return false;
			}
		}

		LoadPipelines();

		return true;
	}

	void ShaderMgr::LoadPipelines()
	{
		{
			_RegisterField(PipelineSettings, inputAssembly.topology)

			_RegisterField(PipelineSettings, rasterizer.cullMode)
			_RegisterField(PipelineSettings, rasterizer.polygonMode)
			_RegisterField(PipelineSettings, rasterizer.frontFace)
			_RegisterField(PipelineSettings, rasterizer.depthBias)
			_RegisterField(PipelineSettings, rasterizer.depthBiasClamp)
			_RegisterField(PipelineSettings, rasterizer.slopeScaledDepthBias)
			_RegisterField(PipelineSettings, rasterizer.enableScissor)

			_RegisterField(PipelineSettings, depthStencil.enableDepthTest)
			_RegisterField(PipelineSettings, depthStencil.enableDepthWrite)
			_RegisterField(PipelineSettings, depthStencil.depthCompareOp)

			_RegisterField(PipelineSettings, blendState.enableBlending)
			_RegisterField(PipelineSettings, blendState.srcColorBlendFactor)
			_RegisterField(PipelineSettings, blendState.dstColorBlendFactor)
			_RegisterField(PipelineSettings, blendState.srcAlphaBlendFactor)
			_RegisterField(PipelineSettings, blendState.dstAlphaBlendFactor)
			_RegisterField(PipelineSettings, blendState.colorBlendOp)
			_RegisterField(PipelineSettings, blendState.alphaBlendOp)
		}

		String path = EngineInfo::GetPaths().ShaderPipelineListFile();
		ConfigFile config;
		if (!config.Load(path))
			return;

		for (auto iter = config.Begin(); iter != config.End(); ++iter)
		{
			const ConfigSection& section = (*iter).second;
			auto& def = _pipelineDefinitions[section.GetName()];
			usize memAddr = (usize)&def.settings;

			for (auto subIter = section.Begin(); subIter != section.End(); ++subIter)
			{
				auto found = _pipelineFields.find((*subIter).first);
				if (found != _pipelineFields.end())
				{
					auto& field = (*found).second;
					if (field.isFloat)
					{
						float fVal = StrToFloat((*subIter).second);
						memcpy((void*)(memAddr + field.offset), &fVal, sizeof(fVal));
					}
					else
					{
						int iVal = StrToInt((*subIter).second);
						memcpy((void*)(memAddr + field.offset), &iVal, sizeof(iVal));
					}
					def.fields.push_back(field);
				}
			}
		}
	}

	void ShaderMgr::RegisterPipelineSettingsField(const String& strField, uint offset, uint size, bool isFloat)
	{
		PipelineField field;
		field.size = size;
		field.offset = offset;
		field.isFloat = isFloat;
		_pipelineFields[strField] = field;
	}

	bool ShaderMgr::BuildPipelineSettings(const String& pipeline, PipelineSettings& settings) const
	{
		auto found = _pipelineDefinitions.find(pipeline);
		if (found != _pipelineDefinitions.end())
		{
			usize srcAddr = (usize)&(*found).second.settings;
			usize dstAddr = (usize)&settings;

			for (auto& field : (*found).second.fields)
				memcpy((void*)(dstAddr + field.offset), (const void*)(srcAddr + field.offset), field.size);
			return true;
		}
		else
		{
			return false;
		}
	}

	ShaderMgr::ShaderMgr()
	{
	}

	ShaderMgr::~ShaderMgr()
	{

	}
}