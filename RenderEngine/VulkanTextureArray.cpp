#include "VulkanTextureArray.h"

namespace SunEngine
{

	VulkanTextureArray::VulkanTextureArray()
	{
	}


	VulkanTextureArray::~VulkanTextureArray()
	{
	}

	bool VulkanTextureArray::Create(const ITextureArrayCreateInfo& info)
	{
		if (info.numImages == 0)
			return false;

		VkExtent2D extent = {};
		extent.width = info.images->image.Width;
		extent.height = info.images->image.Width;

		(void)info;
		VkImageCreateInfo imgInfo = {};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		//imgInfo.flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
		imgInfo.mipLevels = 1 + info.images->mipLevels;
		imgInfo.arrayLayers = info.numImages;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.extent.width = extent.width;
		imgInfo.extent.height = extent.height;
		imgInfo.extent.depth = 1;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		if (info.images->image.Flags & ImageData::COMPRESSED_BC1)
		{
			imgInfo.format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		}
		else if (info.images->image.Flags & ImageData::COMPRESSED_BC3)
		{
			imgInfo.format = VK_FORMAT_BC3_UNORM_BLOCK;
		}

		if (info.images->image.Flags & ImageData::SRGB)
		{
			if (imgInfo.format == VK_FORMAT_R8G8B8A8_UNORM) imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			else if (imgInfo.format == VK_FORMAT_B8G8R8A8_UNORM) imgInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
			else if (imgInfo.format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK) imgInfo.format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			else if (imgInfo.format == VK_FORMAT_BC3_UNORM_BLOCK) imgInfo.format = VK_FORMAT_BC3_SRGB_BLOCK;
		}

		if (!_device->CreateImage(imgInfo, &_image)) return false;
		if (!_device->AllocImageMemory(_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_memory)) return false;

		if (imgInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		{
			Vector<ImageData> images;
			for (uint i = 0; i < info.numImages; i++)
			{
				images.push_back(info.images[i].image);
				for (uint j = 0; j < info.images[i].mipLevels; j++)
					images.push_back(info.images[i].pMips[j]);
			}

			if (!_device->TransferImageData(_image, images.data(), info.numImages, info.images->mipLevels, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) return false;
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
		viewInfo.subresourceRange.layerCount = info.numImages;
		viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		viewInfo.subresourceRange.baseMipLevel = 0;// info.mipLevels ? 3 : 0;

		if (!_device->CreateImageView(viewInfo, &_view)) return false;
		return true;
	}

	bool VulkanTextureArray::Destroy()
	{
		_device->DestroyImageView(_view);
		_device->FreeMemory(_memory);
		_device->DestroyImage(_image);
		return true;
	}

	void VulkanTextureArray::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void VulkanTextureArray::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

}