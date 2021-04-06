#include "StringUtil.h"
#include "ConfigFile.h"
#include "FilePathMgr.h"

namespace SunEngine
{
	EnginePaths& EnginePaths::Get()
	{
		static EnginePaths self;
		return self;
	}

	void EnginePaths::Init(const String& rootDir, ConfigSection* configSection)
	{
		if (configSection == 0)
			return;

		auto& self = Get();
		for (auto iter = configSection->Begin(); iter != configSection->End(); ++iter)
		{
			self._paths[(*iter).first] = rootDir + "\\" + (*iter).second;
		}
	}

	const String& EnginePaths::ShaderSourceDir()
	{
		static String key = "Shaders";
		return Find(key);
	}

	const String& EnginePaths::ShaderListFile()
	{
		static String key = "ShaderList";
		return Find(key);
	}

	const String& EnginePaths::Find(const String& key)
	{
		auto& self = Get();
		static String EMPTY_STR = "";

		auto found = self._paths.find(key);
		return (found != self._paths.end() ? (*found).second : EMPTY_STR);
	}
}