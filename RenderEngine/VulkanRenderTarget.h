#pragma once

#include "IRenderTarget.h"
#include "VulkanRenderOutput.h"

namespace SunEngine
{
	class VulkanTexture;

	class VulkanRenderTarget : public VulkanRenderOutput, public IRenderTarget
	{
	public:
		VulkanRenderTarget();
		~VulkanRenderTarget();

		bool Create(const IRenderTargetCreateInfo &info) override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		void SetClearColor(const float r, const float g, const float b, const float a) override;
		void SetClearOnBind(const bool clear) override;
		void SetViewport(float x, float y, float width, float height) override;

		static VkImageLayout GetColorFinalLayout();
		static VkImageLayout GetDepthFinalLayout();

	private:
		friend class VulkanShaderBindings;
		
		bool createRenderPass(VulkanTexture** pColorTextures, VulkanTexture* pDepthTexture);
		bool createFramebuffer(VulkanTexture** pColorTextures, VulkanTexture* pDepthTexture);

		VkFramebuffer _framebuffer;

		VkClearValue _clearColor;
		VkClearValue _clearDepth;

		VkRect2D _viewport;

		uint _numTargets;
		bool _hasDepth;
		bool _clearOnBind;
		VkRenderPass _noClearRenderPass;
	};

}