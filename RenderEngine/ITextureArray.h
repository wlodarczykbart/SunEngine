#pragma once

#include "IObject.h"

namespace SunEngine
{
	class ITextureArray : public IObject
	{
	public:
		ITextureArray();
		virtual ~ITextureArray();

		virtual bool Create(const ITextureArrayCreateInfo& info) = 0;

		static ITextureArray * Allocate(GraphicsAPI api);
	};
}