#pragma once

#include "ITexture.h"
#include "VulkanObject.h"

namespace SunEngine
{
	class VulkanTexture : public VulkanObject, public ITexture
	{
	public:
		VulkanTexture();
		~VulkanTexture();

		bool Create(const ITextureCreateInfo& info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		inline VkFormat GetFormat() const { return _format; }
		inline VkImageView GetView() const { return _view; }

	private:
		friend class VulkanShaderBindings;

		VkImage _image;
		VkImageView _view;
		VkDeviceMemory _memory;

		VkExtent2D _extent;
		uint _bitsPerPixel;

		VkFormat _format;
	};

}