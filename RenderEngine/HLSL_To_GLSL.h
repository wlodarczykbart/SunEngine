#pragma once

#include "Types.h"

namespace SunEngine
{
	namespace HLSL_To_GLSL
	{
		enum EShaderType
		{
			ST_VERT,
			ST_FRAG,
			ST_GEOM,
		};

		bool Convert(EShaderType type, const String& source, String& convertedSource);
	}
}