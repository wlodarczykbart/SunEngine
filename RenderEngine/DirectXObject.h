#pragma once

#include "DirectXDevice.h"

namespace SunEngine
{
	class DirectXObject
	{
	public:
		virtual ~DirectXObject();
	protected:
		DirectXObject();
		DirectXDevice* _device;
	private:
		friend class DirectXDevice;
	};
}