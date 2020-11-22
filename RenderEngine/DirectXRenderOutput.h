#pragma once

#include "DirectXObject.h"

namespace SunEngine
{
	class DirectXRenderOutput : public DirectXObject
	{
	public:

	protected:
		friend class DirectXGraphicsPipeline;

		DirectXRenderOutput();
		virtual ~DirectXRenderOutput();
	};
}
