#include "VulkanCommandBuffer.h"
#include "VulkanRenderTarget.h"
#include "VulkanTexture.h"

namespace SunEngine
{

	VulkanRenderTarget::VulkanRenderTarget()
	{
		_renderPass = VK_NULL_HANDLE;
		_noClearRenderPass = VK_NULL_HANDLE;
		_framebuffer = VK_NULL_HANDLE;

		_clearColor.color.float32[0] = 0.5f;
		_clearColor.color.float32[1] = 0.5f;
		_clearColor.color.float32[2] = 0.5f;
		_clearColor.color.float32[3] = 0.5f;
		_clearDepth = {};

		_clearOnBind = true;
		_numTargets = 0;
		_hasDepth = false;

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

		VulkanTexture* vkColorTextures[MAX_SUPPORTED_RENDER_TARGETS];
		for (uint i = 0; i < info.numTargets; i++)
			vkColorTextures[i] = static_cast<VulkanTexture*>(info.colorBuffers[i]);

		if (!createRenderPass(vkColorTextures, static_cast<VulkanTexture*>(info.depthBuffer))) return false;
		if (!createFramebuffer(vkColorTextures, static_cast<VulkanTexture*>(info.depthBuffer))) return false;

		_viewport = {};
		_viewport.extent = _extent;

		return true;
	}

	void VulkanRenderTarget::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		uint clearValueCount = 0;
		VkClearValue clearValues[MAX_SUPPORTED_RENDER_TARGETS + 1];

		VkRenderPass renderpass;

		if (_clearOnBind)
		{
			for (uint i = 0; i < _numTargets; i++)
				clearValues[clearValueCount++] = _clearColor;

			if (_hasDepth)
				clearValues[clearValueCount++] = _clearDepth;

			renderpass = _renderPass;
		}
		else
		{
			renderpass = _noClearRenderPass;
		}

		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.framebuffer = _framebuffer;
		info.renderPass = renderpass;
		info.renderArea = _viewport;
		info.clearValueCount = clearValueCount;
		info.pClearValues = clearValues;

		static_cast<VulkanCommandBuffer*>(cmdBuffer)->BeginRenderPass(info, _numTargets);
			
	}

	void VulkanRenderTarget::Unbind(ICommandBuffer * cmdBuffer)
	{
		static_cast<VulkanCommandBuffer*>(cmdBuffer)->EndRenderPass();
	}

	void VulkanRenderTarget::SetClearColor(const float r, const float g, const float b, const float a)
	{
		_clearColor.color.float32[0] = r;
		_clearColor.color.float32[1] = g;
		_clearColor.color.float32[2] = b;
		_clearColor.color.float32[3] = a;
	}

	void VulkanRenderTarget::SetClearOnBind(const bool clear)
	{
		_clearOnBind = clear;
	}

	void VulkanRenderTarget::SetViewport(float x, float y, float width, float height)
	{
		_viewport.extent.width = (uint)width;
		_viewport.extent.height = (uint)height;
		_viewport.offset.x = (uint)x;
		_viewport.offset.y = (uint)y;
	}

	VkImageLayout VulkanRenderTarget::GetColorFinalLayout()
	{
		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkImageLayout VulkanRenderTarget::GetDepthFinalLayout()
	{
		return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
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
			desc.finalLayout = GetColorFinalLayout();
			desc.format = pColorTextures[i]->GetFormat();
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments.push_back(desc);

			VkAttachmentReference ref = {};
			ref.attachment = i;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorRefAttachments.push_back(ref);

			_clearColor.color.float32[0] = 0.5f;
			_clearColor.color.float32[1] = 1.0f;
			_clearColor.color.float32[2] = 0.5f;
			_clearColor.color.float32[3] = 1.0f;
		}


		if (pDepthTexture)
		{
			VkAttachmentDescription desc = {};
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = GetDepthFinalLayout();
			desc.format = pDepthTexture->GetFormat();
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments.push_back(desc);

			VkAttachmentReference ref = {};
			ref.attachment = colorRefAttachments.size();
			ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthRefAttachment.push_back(ref);

			_clearDepth.depthStencil.depth = 1.0f;
			_clearDepth.depthStencil.stencil = 0xff;
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
		Vector<VkImageView> attachments;
		for(uint i = 0; i < _numTargets; i++)
			attachments.push_back(pColorTextures[i]->GetView());
		if (pDepthTexture) attachments.push_back(pDepthTexture->GetView());

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.layers = 1;
		info.renderPass = _renderPass;
		info.width = _extent.width;
		info.height = _extent.height;
		info.attachmentCount = attachments.size();
		info.pAttachments = attachments.data();
		
		if (!_device->CreateFramebuffer(info, &_framebuffer)) return false;

		return true;
	}

}