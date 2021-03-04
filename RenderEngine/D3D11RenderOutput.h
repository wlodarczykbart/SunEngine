#pragma once

#include "D3D11Object.h"

namespace SunEngine
{
	class D3D11RenderOutput : public D3D11Object
	{
	public:

	protected:
		friend class D3D11GraphicsPipeline;

		D3D11RenderOutput();
		virtual ~D3D11RenderOutput();
	};
}
