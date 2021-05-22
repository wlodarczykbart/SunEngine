#pragma once

#include "ITextureCube.h"
#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanTextureCube : public VulkanObject, public ITextureCube
	{
	public:
		VulkanTextureCube();
		~VulkanTextureCube();

		bool Create(const ITextureCubeCreateInfo& info) override;
		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		friend class VulkanShaderBindings;

		VkImage _image;
		VkImageView _view;
		VkDeviceMemory _memory;
	};

}