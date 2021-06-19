#include "VulkanCommandBuffer.h"
#include "VulkanRenderTarget.h"
#include "VulkanTexture.h"

namespace SunEngine
{

	VulkanRenderTarget::VulkanRenderTarget()
	{
		_renderPass = VK_NULL_HANDLE;
		_noClearRenderPass = VK_NULL_HANDLE;
		_numTargets = 0;
		_hasDepth = false;
		_msaaMode = SE_MSAA_OFF;

		_viewport = {};
	}


	VulkanRenderTarget::~VulkanRenderTarget()
	{
	}

	bool VulkanRenderTarget::Create(const IRenderTargetCreateInfo & info)
	{
		_extent.width = info.width;
		_extent.height = info.height;

		_numTargets = info.numTargets;
		_hasDepth = info.depthBuffer != 0;
		_msaaMode = info.msaa;

		VulkanTexture* vkColorTextures[MAX_SUPPORTED_RENDER_TARGETS];
		for (uint i = 0; i < info.numTargets; i++)
			vkColorTextures[i] = static_cast<VulkanTexture*>(info.colorBuffers[i]);

		_framebuffers.resize(info.numLayers);

		if (!createRenderPass(vkColorTextures, static_cast<VulkanTexture*>(info.depthBuffer))) return false;
		if (!createFramebuffer(vkColorTextures, static_cast<VulkanTexture*>(info.depthBuffer))) return false;

		_viewport = {};
		_viewport.extent = _extent;

		return true;
	}

	bool VulkanRenderTarget::Destroy()
	{
		_device->DestroyRenderPass(_renderPass);
		_device->DestroyRenderPass(_noClearRenderPass);
		for(uint i = 0; i < _framebuffers.size(); i++)
			_device->DestroyFramebuffer(_framebuffers[i]);
		_framebuffers.clear();
		return true;
	}

	void VulkanRenderTarget::Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState)
	{
		uint clearValueCount = 0;
		VkClearValue clearValues[MAX_SUPPORTED_RENDER_TARGETS + 1];

		bool clearOnBind = true;
		VkClearValue clearColor = { 0.5f, 1.0f, 0.5f, 1.0f };
		uint layer = 0;

		VkClearValue clearDepth;
		clearDepth.depthStencil.depth = 1.0f;
		clearDepth.depthStencil.stencil = 0xff;

		if (pBindState)
		{
			IRenderTargetBindState* state = static_cast<IRenderTargetBindState*>(pBindState);
			clearOnBind = state->clearOnBind;
			memcpy(clearColor.color.float32, state->clearColor, sizeof(state->clearColor));
			layer = state->layer;
		}

		VkRenderPass renderpass;

		if (clearOnBind)
		{
			for (uint i = 0; i < _numTargets; i++)
				clearValues[clearValueCount++] = clearColor;

			if (_hasDepth)
				clearValues[clearValueCount++] = clearDepth;

			renderpass = _renderPass;
		}
		else
		{
			renderpass = _noClearRenderPass;
		}

		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.framebuffer = _framebuffers[layer];
		info.renderPass = renderpass;
		info.renderArea = _viewport;
		info.clearValueCount = clearValueCount;
		info.pClearValues = clearValues;

		static_cast<VulkanCommandBuffer*>(cmdBuffer)->BeginRenderPass(info, _numTargets, _msaaMode);
			
	}

	void VulkanRenderTarget::Unbind(ICommandBuffer * cmdBuffer)
	{
		static_cast<VulkanCommandBuffer*>(cmdBuffer)->EndRenderPass();
	}

	bool VulkanRenderTarget::createRenderPass(VulkanTexture** pColorTextures, VulkanTexture* pDepthTexture)
	{
		Vector<VkAttachmentDescription> attachments;
		Vector<VkAttachmentReference> colorRefAttachments;
		Vector<VkAttachmentReference> depthRefAttachment;

		for(uint i = 0; i < _numTargets; i++)
		{
			VkAttachmentDescription desc = {};
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = pColorTextures[i]->GetImageLayout();
			desc.format = pColorTextures[i]->GetFormat();
			desc.samples = pColorTextures[i]->GetSampleMask();
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments.push_back(desc);

			VkAttachmentReference ref = {};
			ref.attachment = i;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorRefAttachments.push_back(ref);
		}


		if (pDepthTexture)
		{
			VkAttachmentDescription desc = {};
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = pDepthTexture->GetImageLayout();
			desc.format = pDepthTexture->GetFormat();
			desc.samples = pDepthTexture->GetSampleMask();
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments.push_back(desc);

			VkAttachmentReference ref = {};
			ref.attachment = colorRefAttachments.size();
			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthRefAttachment.push_back(ref);
		}

		Vector<VkSubpassDependency> dependencies;
		dependencies.resize(2);

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;								// Producer of the dependency 
		dependencies[0].dstSubpass = 0;													// Consumer is our single subpass that will wait for the execution depdendency
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;			
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Second dependency at the end the renderpass
		// Does the transition from the initial to the final layout
		dependencies[1].srcSubpass = 0;													// Producer of the dependency is our single subpass
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;								// Consumer are all commands outside of the renderpass
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkSubpassDescription subpass = {};
		subpass.colorAttachmentCount = colorRefAttachments.size();
		subpass.pColorAttachments = colorRefAttachments.data();
		subpass.pDepthStencilAttachment = depthRefAttachment.data();

		VkRenderPassCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		info.attachmentCount = attachments.size();
		info.pAttachments = attachments.data();
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.pDependencies = dependencies.data();
		info.dependencyCount = dependencies.size();
		
		if (!_device->CreateRenderPass(info, &_renderPass)) return false;

		for (uint i = 0; i < attachments.size(); i++)
		{
			//if (attachments[i].format == pColorTexture->GetFormat()) TODO???
				attachments[i].initialLayout = attachments[i].finalLayout;
		}

		if (!_device->CreateRenderPass(info, &_noClearRenderPass)) return false;

		return true;
	}

	bool VulkanRenderTarget::createFramebuffer(VulkanTexture** pColorTextures, VulkanTexture* pDepthTexture)
	{
		for (uint i = 0; i < _framebuffers.size(); i++)
		{
			Vector<VkImageView> attachments;

			for (uint j = 0; j < _numTargets; j++)
				attachments.push_back(pColorTextures[j]->GetLayerView(i));

			if (pDepthTexture) attachments.push_back(pDepthTexture->GetLayerView(i));

			VkFramebufferCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.layers = 1;
			info.renderPass = _renderPass;
			info.width = _extent.width;
			info.height = _extent.height;
			info.attachmentCount = attachments.size();
			info.pAttachments = attachments.data();

			if (!_device->CreateFramebuffer(info, &_framebuffers[i])) return false;
		}

		return true;
	}

}