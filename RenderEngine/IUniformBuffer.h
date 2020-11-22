#pragma once

#include "IObject.h"

namespace SunEngine
{
	class IUniformBuffer : public IObject
	{
	public:
		IUniformBuffer();
		virtual ~IUniformBuffer();

		virtual bool Create(const IUniformBufferCreateInfo& info) = 0;
		virtual bool Update(const void* pData) = 0;
		virtual bool Update(const void* pData, uint offset, uint size) = 0;

		static IUniformBuffer * Allocate(GraphicsAPI api);
	};

}