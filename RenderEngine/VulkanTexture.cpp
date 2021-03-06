#include "VulkanTexture.h"

namespace SunEngine
{

	VulkanTexture::VulkanTexture()
	{
		_image = VK_NULL_HANDLE;
		_view = VK_NULL_HANDLE;
		_cubeToArrayView = VK_NULL_HANDLE;
		_memory = VK_NULL_HANDLE;
		_sampleMask = VK_SAMPLE_COUNT_1_BIT;
		_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		_viewInfo = {};
		_external = false;
	}


	VulkanTexture::~VulkanTexture()
	{
	}

	bool VulkanTexture::Create(const ITextureCreateInfo& info)
	{
		if (info.numImages == 0)
			return false;

		VkExtent2D extent = {};
		extent.width = info.images->image.Width;
		extent.height = info.images->image.Height;

		uint flags = info.images->image.Flags;

		(void)info;
		VkImageCreateInfo imgInfo = {};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imgInfo.mipLevels = 1 + info.images->mipLevels;
		imgInfo.arrayLayers = info.numImages;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.extent.width = extent.width;
		imgInfo.extent.height = extent.height;
		imgInfo.extent.depth = 1;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		if(flags & ImageData::CUBEMAP)
			imgInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		if (flags & ImageData::MULTI_SAMPLES_2) imgInfo.samples = VK_SAMPLE_COUNT_2_BIT;
		else if (flags & ImageData::MULTI_SAMPLES_4) imgInfo.samples = VK_SAMPLE_COUNT_4_BIT;
		else if (flags & ImageData::MULTI_SAMPLES_8) imgInfo.samples = VK_SAMPLE_COUNT_8_BIT;
		else imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		Vector<ImageData> images;
		for (uint i = 0; i < info.numImages; i++)
		{
			if (info.images[i].image.Pixels)
			{
				images.push_back(info.images[i].image);
				for (uint j = 0; j < info.images[i].mipLevels; j++)
					images.push_back(info.images[i].pMips[j]);
			}
		}

		_layout = VK_IMAGE_LAYOUT_UNDEFINED;

		//Assume that a image with input data is a sampled image that will be read only
		if (images.size())
		{
			imgInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		if (flags & ImageData::COLOR_BUFFER_RGBA8)
		{
			imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imgInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
			_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			//TODO: should color buffers have VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL?
		}
		else if (flags & ImageData::COLOR_BUFFER_RGBA16F)
		{
			imgInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imgInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
			_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			//TODO: should color buffers have VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL?
		}
		else if (flags & ImageData::DEPTH_BUFFER)
		{
			imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imgInfo.format = VK_FORMAT_D32_SFLOAT;
			//imgInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
			_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		}
		else if (flags & ImageData::COMPRESSED_BC1)
		{
			imgInfo.format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		}
		else if (flags & ImageData::COMPRESSED_BC3)
		{
			imgInfo.format = VK_FORMAT_BC3_UNORM_BLOCK;
		}
		else if (flags & ImageData::SAMPLED_TEXTURE_R32F)
		{
			imgInfo.format = VK_FORMAT_R32_SFLOAT;
		}
		else if (flags & ImageData::SAMPLED_TEXTURE_R32G32B32A32F)
		{
			imgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT; //TODO: not sure if this needs more work on vulkan side, haven't tested
		}

		if (flags & ImageData::WRITABLE)
			imgInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

		if (flags & ImageData::SRGB)
		{
			if (imgInfo.format == VK_FORMAT_R8G8B8A8_UNORM) imgInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			else if (imgInfo.format == VK_FORMAT_B8G8R8A8_UNORM) imgInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
			else if (imgInfo.format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK) imgInfo.format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			else if (imgInfo.format == VK_FORMAT_BC3_UNORM_BLOCK) imgInfo.format = VK_FORMAT_BC3_SRGB_BLOCK;
		}

		if (!_device->CreateImage(imgInfo, &_image)) return false;
		if (!_device->AllocImageMemory(_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_memory)) return false;

		if (imgInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			if (!_device->TransferImageData(_image, images.data(), info.numImages, info.images->mipLevels, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) return false;

		if (imgInfo.usage & VK_IMAGE_USAGE_STORAGE_BIT)
		{
			if (!_device->SetImageLayout(_image, info.numImages, info.images->mipLevels, _layout, VK_IMAGE_LAYOUT_GENERAL)) return false;
			_layout = VK_IMAGE_LAYOUT_GENERAL;
		}

		_viewInfo = {};
		_viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		_viewInfo.image = _image;
		_viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		_viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		_viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		_viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		_viewInfo.format = imgInfo.format;
		_viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		_viewInfo.subresourceRange.layerCount = info.numImages;
		_viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		_viewInfo.subresourceRange.baseMipLevel = 0;// info.mipLevels ? 3 : 0;

		if (flags & ImageData::CUBEMAP)
			_viewInfo.viewType = info.numImages > 6 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
		else
			_viewInfo.viewType = info.numImages > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

		if (flags & ImageData::COLOR_BUFFER_RGBA8)
		{
			_viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		if (flags & ImageData::COLOR_BUFFER_RGBA16F)
		{
			_viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}
		else if (flags & ImageData::DEPTH_BUFFER)
		{
			_viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		if (!_device->CreateImageView(_viewInfo, &_view)) return false;

		if (flags & ImageData::CUBEMAP)
		{
			VkImageViewCreateInfo cubeToArrayInfo = _viewInfo;
			cubeToArrayInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			if (!_device->CreateImageView(cubeToArrayInfo, &_cubeToArrayView)) return false;
		}

		_sampleMask = imgInfo.samples;

		return true;
	}

	bool VulkanTexture::Destroy()
	{
		_device->DestroyImageView(_view);
		if (!_external)
		{
			_device->FreeMemory(_memory);
			_device->DestroyImage(_image);
		}

		if (_cubeToArrayView != VK_NULL_HANDLE)
		{
			_device->DestroyImageView(_cubeToArrayView);
			_cubeToArrayView = VK_NULL_HANDLE;
		}

		_external = false;
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