#pragma once

#include "ConfigFile.h"
#include "Shader.h"

namespace SunEngine
{
	namespace ShaderCompiler
	{
		bool Compile(int argc, const char** argv, String& errStr, ConfigFile* pConfig = 0);
		//bool Recompile(Shader* pShader);
	}
}