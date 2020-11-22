#pragma once

#include "IObject.h"

namespace SunEngine
{
	class ITextureCube : public IObject
	{
	public:
		ITextureCube();
		virtual ~ITextureCube();

		virtual bool Create(const ITextureCubeCreateInfo& info) = 0;

		static ITextureCube * Allocate(GraphicsAPI api);
	};
}