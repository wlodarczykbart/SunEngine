#include "StringUtil.h"
#include "ConfigFile.h"
#include "FilePathMgr.h"

namespace SunEngine
{
	EngineInfo& EngineInfo::Get()
	{
		static EngineInfo self;
		return self;
	}

	void EngineInfo::Init(ConfigFile* pConfig)
	{
		if (pConfig == 0)
			return;

		auto& self = Get();
		self._paths.Init(pConfig);
		self._renderer.Init(pConfig);
	}

	void EngineInfo::Paths::Init(ConfigFile* pConfig)
	{
		ConfigSection* configSection = pConfig->GetSection("Paths");
		if (configSection == 0)
			return;

		String rootDir = GetDirectory(pConfig->GetFilename()) + "/";
		for (auto iter = configSection->Begin(); iter != configSection->End(); ++iter)
		{
			_paths[(*iter).first] = rootDir + (*iter).second;
		}
	}

	const String& EngineInfo::Paths::ShaderSourceDir() const
	{
		static String key = "Shaders";
		return Find(key);
	}

	const String& EngineInfo::Paths::ShaderListFile() const
	{
		static String key = "ShaderList";
		return Find(key);
	}

	const String& EngineInfo::Paths::Find(const String& key) const
	{
		static String EMPTY_STR = "";

		auto found = _paths.find(key);
		return (found != _paths.end() ? (*found).second : EMPTY_STR);
	}

	void EngineInfo::Renderer::Init(ConfigFile* pConfig)
	{
		ConfigSection* configSection = pConfig->GetSection("Renderer");
		if (configSection == 0)
			return;

		String strAPI = configSection->GetString("API", "vulkan");
		String strRenderMode = configSection->GetString("RenderMode", "forward");

		if (strAPI == "vulkan")
			_api = SE_GFX_VULKAN;
		else if (strAPI == "d3d11")
			_api = SE_GFX_D3D11;

		if (strRenderMode == "forward")
			_renderMode = Forward;
		else if (strRenderMode == "deferred")
			_renderMode = Deferred;

		SetGraphicsAPI(_api);
	}
}