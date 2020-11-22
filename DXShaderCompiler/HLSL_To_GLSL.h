#pragma once

#include "Types.h"

namespace HLSL_To_GLSL
{
	enum EShaderType
	{
		ST_VERT,
		ST_FRAG,
		ST_GEOM,
	};

	bool Convert(EShaderType type, const SunEngine::String& source, SunEngine::String& convertedSource);
}