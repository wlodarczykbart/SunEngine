#include <assert.h>
#include "VulkanShader.h"
#include "VulkanRenderOutput.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRenderTarget.h"
#include "VulkanGraphicsPipeline.h"

namespace SunEngine
{
	Map<PrimitiveTopology, VkPrimitiveTopology> PrimTopologyMap =
	{
		{ SE_PT_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, },
		{ SE_PT_POINT_LIST, VK_PRIMITIVE_TOPOLOGY_POINT_LIST, },
		{ SE_PT_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, },
	};

	Map<CullMode, VkCullModeFlags> CullModeMap =
	{
		{ SE_CM_FRONT, VK_CULL_MODE_FRONT_BIT }, 
		{ SE_CM_BACK, VK_CULL_MODE_BACK_BIT },  
		{ SE_CM_NONE, VK_CULL_MODE_NONE }, 
	};

	Map<PolygonMode, VkPolygonMode> PolygonModeMap =
	{
		{ SE_PM_FILL, VK_POLYGON_MODE_FILL }, 
		{ SE_PM_LINE, VK_POLYGON_MODE_LINE }, 
	};

	//These are inverted because vulkan flips the screen space y...
	Map<FrontFace, VkFrontFace> FrontFaceMap =
	{
		{ SE_FF_CLOCKWISE, VK_FRONT_FACE_CLOCKWISE },
		{ SE_FF_COUNTER_CLOCKWISE, VK_FRONT_FACE_COUNTER_CLOCKWISE },

		//{ SE_FF_CLOCKWISE, VK_FRONT_FACE_COUNTER_CLOCKWISE },
		//{ SE_FF_COUNTER_CLOCKWISE, VK_FRONT_FACE_CLOCKWISE },
	};

	Map<DepthCompareOp, VkCompareOp> DepthCompreOpMap =
	{
		{ SE_DC_LESS, VK_COMPARE_OP_LESS },
		{ SE_DC_LESS_EQUAL, VK_COMPARE_OP_LESS_OR_EQUAL },
		{ SE_DC_EQUAL, VK_COMPARE_OP_EQUAL },
		{ SE_DC_ALWAYS, VK_COMPARE_OP_ALWAYS },
		{ SE_DC_GREATER, VK_COMPARE_OP_GREATER },
		{ SE_DC_NOT_EQUAL, VK_COMPARE_OP_NOT_EQUAL },
	};

	Map<BlendFactor, VkBlendFactor> BlendFactorMap =
	{
		{ SE_BF_ZERO,					VK_BLEND_FACTOR_ZERO },
		{ SE_BF_ONE,					VK_BLEND_FACTOR_ONE },
		{ SE_BF_SRC_COLOR,				VK_BLEND_FACTOR_SRC_COLOR },
		{ SE_BF_DST_COLOR,				VK_BLEND_FACTOR_DST_COLOR },
		{ SE_BF_SRC_ALPHA,				VK_BLEND_FACTOR_SRC_ALPHA },
		{ SE_BF_DST_ALPHA,				VK_BLEND_FACTOR_DST_ALPHA },
		{ SE_BF_ONE_MINUS_SRC_ALPHA,	VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA },
		{ SE_BF_ONE_MINUS_DST_ALPHA,	VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA },
	};

	Map<BlendOp, VkBlendOp> BlendOpMap =
	{
		{ SE_BO_ADD, VK_BLEND_OP_ADD, },
		{ SE_BO_SUBTRACT, VK_BLEND_OP_SUBTRACT, },
	};

	VulkanGraphicsPipeline::VulkanGraphicsPipeline()
	{
		_shader = 0;
	}


	VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
	{
	}

	bool VulkanGraphicsPipeline::Create(const IGraphicsPipelineCreateInfo & info)
	{
		_settings = info.settings;
		_shader = static_cast<VulkanShader*>(info.pShader);

		return true;
	}

	bool VulkanGraphicsPipeline::Destroy()
	{
		Map<VkRenderPass, Map<IShader*, VkPipeline>>::iterator iter = _renderPassPipelines.begin();
		while (iter != _renderPassPipelines.end())
		{
			Map<IShader*, VkPipeline>::iterator passIter = (*iter).second.begin();
			while (passIter != (*iter).second.end())
			{
				_device->DestroyPipeline((*passIter).second);
				++passIter;
			}
			++iter;
		}

		_renderPassPipelines.clear();
		return true;
	}

	void VulkanGraphicsPipeline::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		VulkanCommandBuffer* vkCmd = static_cast<VulkanCommandBuffer*>(cmdBuffer);

		VkPipeline pipeline = VK_NULL_HANDLE;
		Map<VkRenderPass, Map<IShader*, VkPipeline>>::iterator iter = _renderPassPipelines.find(vkCmd->GetCurrentRenderPass());
		if (iter == _renderPassPipelines.end())
		{
			pipeline = createPipeline(vkCmd->GetCurrentRenderPass(), vkCmd->GetCurrentNumTargets());
			_renderPassPipelines[vkCmd->GetCurrentRenderPass()][_shader] = pipeline;
		}
		else
		{
			Map<IShader*, VkPipeline>::iterator passIter = (*iter).second.find(_shader);
			if (passIter == (*iter).second.end())
			{
				pipeline = createPipeline(vkCmd->GetCurrentRenderPass(), vkCmd->GetCurrentNumTargets());
				_renderPassPipelines[vkCmd->GetCurrentRenderPass()][_shader] = pipeline;
			}
			else
			{			
				pipeline = (*passIter).second;
			}
		}

		assert(pipeline != VK_NULL_HANDLE);
		vkCmd->BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	}

	void VulkanGraphicsPipeline::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	VulkanShader * VulkanGraphicsPipeline::GetShader() const
	{
		return _shader;
	}

	void VulkanGraphicsPipeline::TryAddShaderStage(VkShaderModule shader, Vector<VkPipelineShaderStageCreateInfo>& stages, VkShaderStageFlagBits type)
	{
		if (shader != VK_NULL_HANDLE)
		{
			VkPipelineShaderStageCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			info.module = shader;
			info.pName = "main";
			info.stage = type;
			
			stages.push_back(info);
		}
	}

	VkPipeline VulkanGraphicsPipeline::createPipeline(VkRenderPass renderPass, uint numTargets)
	{
		Vector<VkPipelineShaderStageCreateInfo> shaderStages;

		TryAddShaderStage(_shader->_vertShader, shaderStages, VK_SHADER_STAGE_VERTEX_BIT);
		TryAddShaderStage(_shader->_fragShader, shaderStages, VK_SHADER_STAGE_FRAGMENT_BIT);
		TryAddShaderStage(_shader->_geomShader, shaderStages, VK_SHADER_STAGE_GEOMETRY_BIT);

		VkPipelineVertexInputStateCreateInfo      vertexInputState = {};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexAttributeDescriptionCount = _shader->_inputAttribs.size();
		vertexInputState.pVertexAttributeDescriptions = _shader->_inputAttribs.data();
		if (_shader->_inputAttribs.size())
		{
			vertexInputState.vertexBindingDescriptionCount = 1;
			vertexInputState.pVertexBindingDescriptions = &_shader->_inputBinding;
		}

		VkPipelineInputAssemblyStateCreateInfo    inputAssemblyState = {};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = PrimTopologyMap[_settings.inputAssembly.topology];

		//VkPipelineTessellationStateCreateInfo     tessellationState = {};

		VkPipelineViewportStateCreateInfo         viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo    rasterizationState = {};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.cullMode = CullModeMap[_settings.rasterizer.cullMode];
		rasterizationState.frontFace = FrontFaceMap[_settings.rasterizer.frontFace];
		rasterizationState.polygonMode = PolygonModeMap[_settings.rasterizer.polygonMode];
		rasterizationState.lineWidth = 1.0f;
		rasterizationState.depthBiasConstantFactor = _settings.rasterizer.depthBias;
		rasterizationState.depthBiasClamp = _settings.rasterizer.depthBiasClamp;
		rasterizationState.depthBiasSlopeFactor = _settings.rasterizer.slopeScaledDepthBias;
		rasterizationState.depthBiasEnable = _settings.rasterizer.depthBias != 0 || _settings.rasterizer.slopeScaledDepthBias != 0;

		VkPipelineMultisampleStateCreateInfo      multisamplerState = {};
		multisamplerState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplerState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo     depthStencilState = {};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = _settings.depthStencil.enableDepthTest;
		depthStencilState.depthWriteEnable = _settings.depthStencil.enableDepthWrite;
		depthStencilState.depthCompareOp = DepthCompreOpMap[_settings.depthStencil.depthCompareOp];

		VkPipelineColorBlendStateCreateInfo       colorBlendState = {};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		VkPipelineColorBlendAttachmentState blendAttachments[MAX_SUPPORTED_RENDER_TARGETS];
		for (uint i = 0; i < numTargets; i++)
		{
			blendAttachments[i].blendEnable = _settings.blendState.enableBlending;
			blendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendAttachments[i].srcColorBlendFactor = BlendFactorMap[_settings.blendState.srcColorBlendFactor];
			blendAttachments[i].dstColorBlendFactor = BlendFactorMap[_settings.blendState.dstColorBlendFactor];
			blendAttachments[i].srcAlphaBlendFactor = BlendFactorMap[_settings.blendState.srcAlphaBlendFactor];
			blendAttachments[i].dstAlphaBlendFactor = BlendFactorMap[_settings.blendState.dstAlphaBlendFactor];
			blendAttachments[i].colorBlendOp = BlendOpMap[_settings.blendState.colorBlendOp];
			blendAttachments[i].alphaBlendOp = BlendOpMap[_settings.blendState.alphaBlendOp];
		}
		colorBlendState.attachmentCount = numTargets;
		colorBlendState.pAttachments = blendAttachments;

		Vector<VkDynamicState> states;
		states.push_back(VK_DYNAMIC_STATE_SCISSOR);
		states.push_back(VK_DYNAMIC_STATE_VIEWPORT);

		VkPipelineDynamicStateCreateInfo          dynamicState = {};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = states.size();
		dynamicState.pDynamicStates = states.data();


		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = _shader->_layout;
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		//pipelineInfo.pTessellationState =	&tessellationState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pMultisampleState = &multisamplerState;
		pipelineInfo.pDepthStencilState = &depthStencilState;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.stageCount = shaderStages.size();
		pipelineInfo.pStages = shaderStages.data();

		VkPipeline pipeline;

		pipelineInfo.renderPass = renderPass;
		if (!_device->CreateGraphicsPipeline(pipelineInfo, &pipeline)) return VK_NULL_HANDLE;

		return pipeline;
	}

}