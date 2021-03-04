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
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

		if (settings.anisotropicMode != SE_AM_OFF)
		{
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = float(1 << settings.anisotropicMode);
		}

		if (!_device->CreateSampler(samplerInfo, &_sampler)) return false;

		return true;
	}

	bool VulkanSampler::Destroy()
	{
		_device->DestroySampler(_sampler);
		return true;
	}

	void VulkanSampler::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void VulkanSampler::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

}