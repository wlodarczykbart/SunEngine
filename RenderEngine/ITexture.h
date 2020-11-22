#pragma once

#include "IObject.h"

namespace SunEngine
{
	class ITexture : public IObject
	{
	public:
		ITexture();
		virtual ~ITexture();

		virtual bool Create(const ITextureCreateInfo& info) = 0;

		static ITexture* Allocate(GraphicsAPI api);
	};
}