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
		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		friend class VulkanShaderBindings;

		VkImage _images[6];
		VkImageView _view;
	};

}