#include "VulkanTexture.h"

namespace SunEngine
{
	VulkanTexture::VulkanTexture()
	{
		_image = VK_NULL_HANDLE;
		_view = VK_NULL_HANDLE;
		_memory = VK_NULL_HANDLE;
		_format = VK_FORMAT_UNDEFINED;
		_sampleMask = VK_SAMPLE_COUNT_1_BIT;
		_layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}


	VulkanTexture::~VulkanTexture()
	{
	}

	bool VulkanTexture::Create(const ITextureCreateInfo & info)
	{
		VkImageCreateInfo vkInfo = {};
		vkInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		vkInfo.extent.width = info.image.Width;
		vkInfo.extent.height = info.image.Height;
		vkInfo.extent.depth = 1;
		vkInfo.arrayLayers = 1;
		vkInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		vkInfo.imageType = VK_IMAGE_TYPE_2D;
		vkInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.mipLevels = info.mipLevels + 1;
		vkInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		vkInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (info.image.Flags & ImageData::MULTI_SAMPLES_2) vkInfo.samples = VK_SAMPLE_COUNT_2_BIT;
		else if (info.image.Flags & ImageData::MULTI_SAMPLES_4) vkInfo.samples = VK_SAMPLE_COUNT_4_BIT;
		else if (info.image.Flags & ImageData::MULTI_SAMPLES_8) vkInfo.samples = VK_SAMPLE_COUNT_8_BIT;
		else vkInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		
		if (info.image.Flags & ImageData::COLOR_BUFFER_RGBA8)
		{
			vkInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			vkInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
			//TODO: should color buffers have VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL?
		}
		else if (info.image.Flags & ImageData::COLOR_BUFFER_RGBA16F)
		{
			vkInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			vkInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			//TODO: should color buffers have VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL?
		}
		else if (info.image.Flags & ImageData::DEPTH_BUFFER)
		{
			vkInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			vkInfo.format = VK_FORMAT_D32_SFLOAT;
			//vkInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
			_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
		else if (info.image.Flags & ImageData::COMPRESSED_BC1)
		{
			vkInfo.format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		}
		else if (info.image.Flags & ImageData::COMPRESSED_BC3)
		{
			vkInfo.format = VK_FORMAT_BC3_UNORM_BLOCK;
		}
		else if (info.image.Flags & ImageData::SAMPLED_TEXTURE_R32F)
		{
			vkInfo.format = VK_FORMAT_R32_SFLOAT;
		}
		else if (info.image.Flags & ImageData::SAMPLED_TEXTURE_R32G32B32A32F)
		{
			vkInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT; //TODO: not sure if this needs more work on vulkan side, haven't tested
		}

		if (info.image.Flags & ImageData::SRGB)
		{
			if (vkInfo.format == VK_FORMAT_R8G8B8A8_UNORM) vkInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			else if (vkInfo.format == VK_FORMAT_B8G8R8A8_UNORM) vkInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
			else if (vkInfo.format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK) vkInfo.format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			else if (vkInfo.format == VK_FORMAT_BC3_UNORM_BLOCK) vkInfo.format  = VK_FORMAT_BC3_SRGB_BLOCK;
		}
		
		if (!_device->CreateImage(vkInfo, &_image)) return false;
		if (!_device->AllocImageMemory(_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_memory)) return false;

		if (vkInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		{
			Vector<ImageData> images;
			images.push_back(info.image);
			for (uint i = 0; i < info.mipLevels; i++)
				images.push_back(info.pMips[i]);

			if (!_device->TransferImageData(_image, images.data(), 1, info.mipLevels, _layout)) return false;
		}

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = _image;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		viewInfo.format = vkInfo.format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.baseMipLevel = 0;// info.mipLevels ? 3 : 0;

		if (info.image.Flags & ImageData::COLOR_BUFFER_RGBA8)
		{
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (info.image.Flags & ImageData::COLOR_BUFFER_RGBA16F)
		{
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else if (info.image.Flags & ImageData::DEPTH_BUFFER)
		{
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		if (!_device->CreateImageView(viewInfo, &_view)) return false;

		_format = viewInfo.format;
		_sampleMask = vkInfo.samples;

		return true;
	}

	bool VulkanTexture::Destroy()
	{
		_device->DestroyImageView(_view);
		_device->FreeMemory(_memory);
		_device->DestroyImage(_image);
		return true;
	}

	void VulkanTexture::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void VulkanTexture::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}
}