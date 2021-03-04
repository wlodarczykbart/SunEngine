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
		return true;


		if (info.numImages == 0)
			return false;

		uint width = info.pImages[0].image.Width;
		uint height = info.pImages[0].image.Height;

		(void)info;
		VkImageCreateInfo imgInfo = {};
		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.imageType = VK_IMAGE_TYPE_3D;
		imgInfo.flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
		imgInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imgInfo.mipLevels = 1;
		imgInfo.arrayLayers = 1;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imgInfo.extent.width = width;
		imgInfo.extent.height = height;
		imgInfo.extent.depth = 1;
		// Set initial layout of the image to undefined
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		VkImage transfer;
		if (!_device->CreateImage(imgInfo, &transfer))
			return false;

		//_device->AllocImageMemory(transfer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

		//VkImageViewCreateInfo viewInfo = {};
		//viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

		_device->TransferImageData(transfer, info.pImages[0].image, 0, 0);
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