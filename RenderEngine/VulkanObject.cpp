#include "GraphicsContext.h"
#include "VulkanObject.h"

namespace SunEngine
{

	VulkanObject::VulkanObject()
	{
		_device = static_cast<VulkanDevice*>(GraphicsContext::GetDevice());
	}


	VulkanObject::~VulkanObject()
	{

	}

}