#include "VulkanSampler.h"

namespace SunEngine
{
	Map<FilterMode, VkFilter> FilterMap
	{
		{ SE_FM_NEAREST, VK_FILTER_NEAREST },
		{ SE_FM_LINEAR, VK_FILTER_LINEAR }
	};

	Map<FilterMode, VkSamplerMipmapMode> MipmapMap
	{
		{ SE_FM_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST },
		{ SE_FM_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR }
	};

	Map<WrapMode, VkSamplerAddressMode> AddressMap
	{
		{ SE_WM_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE },
		{ SE_WM_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT }
	};

	VulkanSampler::VulkanSampler()
	{
	}

	VulkanSampler::~VulkanSampler()
	{
	}

	bool VulkanSampler::Create(const ISamplerCreateInfo & info)
	{
		SamplerSettings settings = info.settings;

		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.addressModeU = AddressMap[settings.wrapMode];
		samplerInfo.addressModeV = AddressMap[settings.wrapMode];
		samplerInfo.addressModeW = AddressMap[settings.wrapMode];
		samplerInfo.minFilter = FilterMap[settings.filterMode];
		samplerInfo.magFilter = FilterMap[settings.filterMode];
		samplerInfo.mipmapMode = MipmapMap[settings.filterMode];

		if (!_device->CreateSampler(samplerInfo, &_sampler)) return false;

		return true;
	}

	bool VulkanSampler::Destroy()
	{
		_device->DestroySampler(_sampler);
		return true;
	}

	void VulkanSampler::Bind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	void VulkanSampler::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

}