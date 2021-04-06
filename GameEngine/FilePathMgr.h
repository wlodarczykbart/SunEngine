#pragma once

#include "Types.h"

namespace SunEngine
{
	class ConfigSection;

	class EnginePaths
	{
	public:
		static const String& ShaderSourceDir();
		static const String& ShaderListFile();

		static const String& Find(const String& key);

		static void Init(const String& rootDir, ConfigSection* configSection);

	private:
		static EnginePaths& Get();

		EnginePaths() = default;
		EnginePaths(const EnginePaths&) = delete;
		EnginePaths& operator = (const EnginePaths&) = delete;
		~EnginePaths() = default;

		StrMap<String> _paths;
	};
}