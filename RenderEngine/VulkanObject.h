#pragma once

#include "VulkanDevice.h"
namespace SunEngine
{
	class VulkanObject
	{
	public:
		virtual ~VulkanObject();
	protected:
		VulkanObject();
		VulkanDevice* _device;
	private:
		friend class VulkanDevice;
	};

}