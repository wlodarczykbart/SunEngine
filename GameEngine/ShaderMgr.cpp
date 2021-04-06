#include "StringUtil.h"
#include "FilePathMgr.h"
#include "ResourceMgr.h"
#include "ShaderMgr.h"

namespace SunEngine
{
	DefineStaticStr(DefaultShaders, Metallic)
	DefineStaticStr(DefaultShaders, Specular)
	DefineStaticStr(DefaultShaders, Gamma)
				
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

	bool ShaderMgr::LoadShaders()
	{
		String path = EnginePaths::ShaderListFile();
		ConfigFile config;
		if (!config.Load(path))
			return false;
		
		String shaderDir = EnginePaths::ShaderSourceDir();

		ConfigSection* pList = config.GetSection("Shaders");
		for (auto iter = pList->Begin(); iter != pList->End(); ++iter)
		{
			String shaderName = GetFileNameNoExt((*iter).first);
			if (_shaders.find(shaderName) != _shaders.end())
				continue;

			String shaderConfig = shaderDir + (*iter).first;
			Shader* pShader = new Shader();
			_shaders[shaderName] = UniquePtr<Shader>(pShader);

			if (!pShader->Compile(shaderConfig))
				return false;
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