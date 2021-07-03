#pragma once

#include "IShader.h"
#include "IShaderBindings.h"
#include "VulkanObject.h"
#include "MemBuffer.h"

namespace SunEngine
{
	class VulkanRenderTarget;
	class VulkanSampler;
	class VulkanUniformBuffer;
	class VulkanTexture;
	class VulkanTextureCube;
	class VulkanCommandBuffer;
	class ConfigFile;

	class VulkanShader;

	class VulkanShaderBindings : public VulkanObject, public IShaderBindings
	{
	public:
		VulkanShaderBindings();
		~VulkanShaderBindings();

		bool Create(const IShaderBindingCreateInfo& createInfo) override;
		bool Destroy() override;

		void Bind(ICommandBuffer *pCmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer *pCmdBuffer) override;

		void SetUniformBuffer(IUniformBuffer* pBuffer, const String& name) override;
		void SetTexture(ITexture* pTexture, const String& name) override;
		void SetSampler(ISampler* pSampler, const String& name) override;

	private:
		struct ResourceBindData
		{
			ResourceBindData()
			{
				pObject = 0;
			}

			IShaderResource resource;
			VulkanObject* pObject;
		};

		VkDescriptorSet GetCurrentSet() const;

		uint _setNumber;
		Vector<VkDescriptorSet> _sets;
		VkDescriptorSetLayout _layout;
		StrMap<ResourceBindData> _bindingMap;
		Vector<uint> _dynamicOffsets;
		uint _firstBuffer;

		Map<uint, VkImageMemoryBarrier> _computeBarriers;
	};

	class VulkanShader : public VulkanObject, public IShader
	{
	public:
		VulkanShader();
		~VulkanShader();

		bool Create(IShaderCreateInfo& info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState*) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		VkPipelineLayout GetPipelineLayout() const;
		VkDescriptorSetLayout GetDescriptorSetLayout(uint set) const;

		//VkDescriptorSetLayoutBinding GetDescriptorSetLayoutBinding(DescriptorSet set, uint binding) const;

		bool IsComputeShader() const;

	private:
		enum BindingDataType
		{
			BDT_UNDEFINED,
			BDT_TEXTURE,
			BDT_COLOR_BUFFER,
			BDT_DEPTH_BUFFER,
			BDT_SAMPLER,
		};

		struct BindingData
		{
			BindingDataType type;
			uint binding;
			VulkanObject* pObject;
		};

		struct UniformBufferData
		{
			uint binding;
			VkDescriptorSet set;
			uint dynamicOffset;
		};

		static int SortBindings(const void* lhs, const void* rhs);
		static const uint MAX_BINDINGS_PER_DRAW = 64;

		friend class VulkanGraphicsPipeline;
		//void SetUniformBuffer(VulkanUniformBuffer* pBuffer, const uint binding);

		//bool GetSetIndex(VkDescriptorType type, uint& set);
		//bool ObjectIsCurrentlyBound(const uint set, const uint binding, uint64_t apiHandle);

		VkShaderModule _vertShader;
		VkShaderModule _fragShader;
		VkShaderModule _geomShader;
		VkShaderModule _compShader;

		Vector<VkVertexInputAttributeDescription> _inputAttribs;
		VkVertexInputBindingDescription _inputBinding;

		Vector<VkDescriptorSetLayout> _setLayouts;

		//Map<VulkanUniformBuffer*, uint> _activeBuffersMap;
		//Map<uint, uint> _bindingToSortedIndexMap;
		//Vector<uint> _dynamicOffsetTable;

		VkPipelineLayout _layout;
		VkPipeline _computePipeline; //compute has no reason to need varying pipeline states from the same shader(I think?), so one pipeline exists here

		BindingData _previousBindings[MAX_BINDINGS_PER_DRAW];
		uint _prevBindingCount;

		BindingData _currentBindings[MAX_BINDINGS_PER_DRAW];
		uint _currBindingCount;


	};
}