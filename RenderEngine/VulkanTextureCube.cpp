#include "VulkanTextureCube.h"

namespace SunEngine
{

	VulkanTextureCube::VulkanTextureCube()
	{
	}


	VulkanTextureCube::~VulkanTextureCube()
	{
	}

	bool VulkanTextureCube::Create(const ITextureCubeCreateInfo & info)
	{
		VkExtent2D extent = {};
		extent.width = info.images->Width;
		extent.height = info.images->Height;

		VkImageCreateInfo imgInfo = {};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.extent.width = extent.width;
		imgInfo.extent.height = extent.height;
		imgInfo.extent.depth = 1;
		imgInfo.arrayLayers = 6;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.mipLevels = 1;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imgInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		if (info.images->Flags & ImageData::COMPRESSED_BC1)
		{
			imgInfo.format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		}
		else if (info.images->Flags & ImageData::COMPRESSED_BC3)
		{
			imgInfo.format = VK_FORMAT_BC3_UNORM_BLOCK;
		}

		if (info.images->Flags & ImageData::SRGB)
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
			if (!_device->TransferImageData(_image, info.images, 6)) return false;
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
		viewInfo.subresourceRange.layerCount = 6;
		viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		viewInfo.subresourceRange.baseMipLevel = 0;// info.mipLevels ? 3 : 0;

		if (!_device->CreateImageView(viewInfo, &_view)) return false;
		return true;
	}

	void VulkanTextureCube::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void VulkanTextureCube::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

}