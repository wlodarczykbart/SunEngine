#pragma once

#include "D3D11Device.h"

namespace SunEngine
{
	class D3D11Object
	{
	public:
		virtual ~D3D11Object();
	protected:
		D3D11Object();
		D3D11Device* _device;
	private:
		friend class D3D11Device;
	};
}