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
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
	private:
		friend class VulkanShaderBindings;
		
		bool createRenderPass(VulkanTexture** pColorTextures, VulkanTexture* pDepthTexture);
		bool createFramebuffer(VulkanTexture** pColorTextures, VulkanTexture* pDepthTexture);

		Vector<VkFramebuffer> _framebuffers;
		Vector<VkImageView> _framebufferViews;

		VkRect2D _viewport;

		uint _numTargets;
		bool _hasDepth;
		MSAAMode _msaaMode;
		VkRenderPass _noClearRenderPass;
	};

}