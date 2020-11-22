#pragma once

#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanRenderOutput : public VulkanObject
	{
	protected:
		friend class VulkanGraphicsPipeline;

		VulkanRenderOutput();
		virtual ~VulkanRenderOutput();

		VkRenderPass _renderPass;
		VkExtent2D _extent;
	};
}