#pragma once

#include "IObject.h"

namespace SunEngine
{
	class ISampler : public IObject
	{
	public:
		ISampler();
		virtual ~ISampler();

		virtual bool Create(const ISamplerCreateInfo& info) = 0;

		static ISampler* Allocate(GraphicsAPI api);
	};
}