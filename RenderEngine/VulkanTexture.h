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
		inline VkImageView GetView() const { return _view; }
		inline VkFormat GetFormat() const { return _format; }
		inline VkSampleCountFlagBits GetSampleMask() const { return _sampleMask; }
		inline VkImageLayout GetImageLayout() const { return _layout; }
		VkImageView GetLayerView(uint layer) const;
	protected:
		friend class VulkanShaderBindings;
		friend class VulkanVRInterface;

		VkImage _image;
		VkImageView _view;
		Vector<VkImageView> _layerViews;
		VulkanDevice::MemoryHandle _memory;
		VkFormat _format;
		VkSampleCountFlagBits _sampleMask;
		VkImageLayout _layout;
	};
}