#include "StringUtil.h"
#include "FilePathMgr.h"
#include "ResourceMgr.h"
#include "ShaderMgr.h"

namespace SunEngine
{
	DefineStaticStr(DefaultShaders, Metallic)
	DefineStaticStr(DefaultShaders, Specular)
	DefineStaticStr(DefaultShaders, Gamma)
	DefineStaticStr(DefaultShaders, Deferred)
	DefineStaticStr(DefaultShaders, ScreenSpaceReflection)
	DefineStaticStr(DefaultShaders, SceneCopy)

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

		return true;
	}

	ShaderMgr::ShaderMgr()
	{
	}

	ShaderMgr::~ShaderMgr()
	{

	}
}