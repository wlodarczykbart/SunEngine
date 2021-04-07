#pragma once

#include "VulkanObject.h"
#include "IGraphicsPipeline.h"

namespace SunEngine
{
	class VulkanRenderOutput;
	class VulkanShader;

	class VulkanGraphicsPipeline : public VulkanObject, public IGraphicsPipeline
	{
	public:
		VulkanGraphicsPipeline();
		~VulkanGraphicsPipeline();

		bool Create(const IGraphicsPipelineCreateInfo &info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		VulkanShader* GetShader() const;

	private:
		VkPipeline createPipeline(VkRenderPass renderPass, uint numTargets);

		void TryAddShaderStage(VkShaderModule shader, Vector<VkPipelineShaderStageCreateInfo> &stages, VkShaderStageFlagBits type);
		Map<VkRenderPass, Map<IShader*, VkPipeline>> _renderPassPipelines;
		VulkanShader* _shader;
		PipelineSettings _settings;
	};

}