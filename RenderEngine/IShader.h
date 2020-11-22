#pragma once

#include "IObject.h"

namespace SunEngine
{
	class IShader : public IObject
	{
	public:
		IShader();
		virtual ~IShader();

		virtual bool Create(IShaderCreateInfo& info) = 0;

		static IShader * Allocate(GraphicsAPI api);
	};

}