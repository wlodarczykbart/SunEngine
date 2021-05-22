#include "VulkanTexture.h"

namespace SunEngine
{
	VulkanTexture::VulkanTexture()
	{
		_image = VK_NULL_HANDLE;
		_view = VK_NULL_HANDLE;
		_memory = VK_NULL_HANDLE;

		_extent = {};
		_bitsPerPixel = 0;

		_format = VK_FORMAT_UNDEFINED;
	}


	VulkanTexture::~VulkanTexture()
	{
	}

	bool VulkanTexture::Create(const ITextureCreateInfo & info)
	{
		_extent.width = info.image.Width;
		_extent.height = info.image.Height;
		_bitsPerPixel = sizeof(Pixel) * 8;
		
		VkImageCreateInfo imgInfo = {};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.extent.width = _extent.width;
		imgInfo.extent.height = _extent.height;
		imgInfo.extent.depth = 1;
		imgInfo.arrayLayers = 1;
		imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.mipLevels = info.mipLevels + 1;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (info.image.Flags & ImageData::COLOR_BUFFER_RGBA8)
		{
			imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imgInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
		}
		else if (info.image.Flags & ImageData::COLOR_BUFFER_RGBA16F)
		{
			imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imgInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		}
		else if (info.image.Flags & ImageData::DEPTH_BUFFER)
		{
			imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imgInfo.format = VK_FORMAT_D32_SFLOAT;
			//imgInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
		}
		else if (info.image.Flags & ImageData::COMPRESSED_BC1)
		{
			imgInfo.format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		}
		else if (info.image.Flags & ImageData::COMPRESSED_BC3)
		{
			imgInfo.format = VK_FORMAT_BC3_UNORM_BLOCK;
		}
		else if (info.image.Flags & ImageData::SAMPLED_TEXTURE_R32F)
		{
			imgInfo.format = VK_FORMAT_R32_SFLOAT;
		}
		else if (info.image.Flags & ImageData::SAMPLED_TEXTURE_R32G32B32A32F)
		{
			imgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT; //TODO: not sure if this needs more work on vulkan side, haven't tested
		}

		if (info.image.Flags & ImageData::SRGB)
		{
			if (imgInfo.format == VK_FORMAT_R8G8B8A8_UNORM) imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			else if (imgInfo.format == VK_FORMAT_B8G8R8A8_UNORM) imgInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
			else if (imgInfo.format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK) imgInfo.format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			else if (imgInfo.format == VK_FORMAT_BC3_UNORM_BLOCK) imgInfo.format  = VK_FORMAT_BC3_SRGB_BLOCK;
		}
		
		if (!_device->CreateImage(imgInfo, &_image)) return false;
		if (!_device->AllocImageMemory(_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_memory)) return false;

		if (imgInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		{
			if (!_device->TransferImageData(_image, info.image, info.mipLevels, info.pMips)) return false;
		}

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = _image;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		viewInfo.format = imgInfo.format;
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