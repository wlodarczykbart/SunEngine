#pragma once

#include "IObject.h"

namespace SunEngine
{
	class IGraphicsPipeline : public IObject
	{
	public:
		IGraphicsPipeline();
		virtual ~IGraphicsPipeline();

		virtual bool Create(const IGraphicsPipelineCreateInfo &info) = 0;

		static IGraphicsPipeline * Allocate(GraphicsAPI api);
	};
}