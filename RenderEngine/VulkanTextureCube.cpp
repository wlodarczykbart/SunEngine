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
		(void)info;
		VkImageCreateInfo imgInfo = {};
		imgInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		//imgInfo.

		return false;

		//VkImageViewCreateInfo viewInfo = {};
		//viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

		//_device->TransferImageData()
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