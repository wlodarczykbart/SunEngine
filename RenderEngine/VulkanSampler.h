#pragma once

#include "ISampler.h"
#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanSampler : public VulkanObject, public ISampler
	{
	public:

		VulkanSampler();
		~VulkanSampler();

		bool Create(const ISamplerCreateInfo &info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		friend class VulkanShaderBindings;
		VkSampler _sampler;
	};
}