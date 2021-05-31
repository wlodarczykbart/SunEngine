#include <string>       // std::string
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <mutex>

#include "StringUtil.h"
#include "FilePathMgr.h"
#include "ResourceMgr.h"
#include "ThreadPool.h"
#include "ShaderCompiler.h"
#include "Timer.h"
#include "ShaderMgr.h"

namespace SunEngine
{
#define _RegisterField(type, field) { type obj; memset(&obj, 0x1, sizeof(obj)); std::stringstream ss; ss << obj.field; Register##type##Field(#field, (usize)(&obj.field) - (usize)&obj, sizeof(obj.field), StrContains(ss.str(), ".")); }

	DefineStaticStr(DefaultShaders, Metallic)
	DefineStaticStr(DefaultShaders, Specular)
	DefineStaticStr(DefaultShaders, ToneMap)
	DefineStaticStr(DefaultShaders, Deferred)
	DefineStaticStr(DefaultShaders, ScreenSpaceReflection)
	DefineStaticStr(DefaultShaders, SceneCopy)
	DefineStaticStr(DefaultShaders, FXAA)
	DefineStaticStr(DefaultShaders, TextureCopy)
	DefineStaticStr(DefaultShaders, Skybox)
	DefineStaticStr(DefaultShaders, Clouds)
	DefineStaticStr(DefaultShaders, SkyArHosek)
	DefineStaticStr(DefaultShaders, MSAAResolve)

	DefineStaticStr(DefaultPipelines, ShadowDepth)

//#define LOAD_SHADER_THREADED 

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
		ShaderCompiler::SetAuxiliaryDir(shaderDir);

#ifdef LOAD_SHADER_THREADED
		ThreadPool& tp = ThreadPool::Get();
#endif
		struct ThreadData
		{
			std::mutex mtx;
			Queue<Pair<String, Shader*>> queue;
			Vector<String> defines;
			String errorMessage;
		} threadData;

		//Make sure these line up with what is in the shaders...
		threadData.defines.push_back(StrFormat("MAX_SKINNED_BONES %d", EngineInfo::GetRenderer().MaxSkinnedBoneMatrices()));
		threadData.defines.push_back(StrFormat("MAX_TEXTURE_TRANSFORMS %d", EngineInfo::GetRenderer().MaxTextureTransforms()));
		threadData.defines.push_back(StrFormat("CASCADE_SHADOW_MAP_SPLITS %d", EngineInfo::GetRenderer().CascadeShadowMapSplits()));
		threadData.defines.push_back(StrFormat("INV_CASCADE_SHADOW_MAP_SPLITS %f", 1.0f / EngineInfo::GetRenderer().CascadeShadowMapSplits()));
		threadData.defines.push_back(StrFormat("CASCADE_SHADOW_MAP_RESOLUTION %d", EngineInfo::GetRenderer().CascadeShadowMapResolution()));
		threadData.defines.push_back(StrFormat("INV_CASCADE_SHADOW_MAP_RESOLUTION %f", 1.0f / EngineInfo::GetRenderer().CascadeShadowMapResolution()));
		threadData.defines.push_back(StrFormat("CASCADE_SHADOW_MAP_PCF_BLUR_SIZE %d", EngineInfo::GetRenderer().CascadeShadowMapPCFBlurSize()));
		threadData.defines.push_back(StrFormat("CASCADE_SHADOW_MAP_PCF_BLUR_SIZE_2 %d", EngineInfo::GetRenderer().CascadeShadowMapPCFBlurSize()*EngineInfo::GetRenderer().CascadeShadowMapPCFBlurSize()));
		threadData.defines.push_back(StrFormat("MSAA_SAMPLES %d", 1 << EngineInfo::GetRenderer().GetMSAAMode()));

		Timer timer(true);

		ConfigSection* pList = config.GetSection("Shaders");
		for (auto iter = pList->Begin(); iter != pList->End(); ++iter)
		{
			String shaderName = GetFileNameNoExt((*iter).first);
			if (_shaders.find(shaderName) != _shaders.end())
				continue;

			Shader* pShader = new Shader();
			_shaders[shaderName] = UniquePtr<Shader>(pShader);		
			{
				std::lock_guard<std::mutex> lock(threadData.mtx);
				threadData.queue.push({ shaderDir + (*iter).first,  pShader });
			}

#ifdef LOAD_SHADER_THREADED
			tp.AddTask([](uint, void* pData) -> void {
				ThreadData* pThreadData = static_cast<ThreadData*>(pData);
				Pair<String, Shader*> compileInfo = {};
				{
					std::lock_guard<std::mutex> lock(pThreadData->mtx);
					//once an error has been found, we no longer attempt to compile any shaders, allow the thread pool to complete these empty tasks
					if (pThreadData->errorMessage.empty())
					{
						compileInfo = pThreadData->queue.front();
						pThreadData->queue.pop();
					}
				}

				if (compileInfo.second)
				{
					String errMsg;
					if (!compileInfo.second->Compile(compileInfo.first, &errMsg, &pThreadData->defines))
					{
						errMsg = compileInfo.first + "\n" + errMsg;
						std::lock_guard<std::mutex> lock(pThreadData->mtx);
						pThreadData->errorMessage = errMsg;
					}
				}

			}, &threadData);
#else
			String shaderConfig = shaderDir + (*iter).first;
			if (!pShader->Compile(shaderConfig, &errMsg, &threadData.defines))
			{
				errMsg = shaderConfig + "\n" + errMsg;
				return false;
			}
#endif
		}

#ifdef LOAD_SHADER_THREADED
		tp.Wait();
		if (!threadData.errorMessage.empty())
		{	
			errMsg = threadData.errorMessage;
			return false;
		}
#endif

		timer.Tick();
		printf("ShaderLoad: %f\n", timer.ElapsedTime());

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