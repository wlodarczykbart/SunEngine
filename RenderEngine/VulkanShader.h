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

		bool Create(IShader* pShader, const Vector<IShaderResource>& textureBindings, const Vector<IShaderResource>& samplerBindings) override;

		void Bind(ICommandBuffer *pCmdBuffer) override;
		void Unbind(ICommandBuffer *pCmdBuffer) override;

		void SetTexture(ITexture* pTexture, uint binding) override;
		void SetSampler(ISampler* pSampler, uint binding) override;
		void SetTextureCube(ITextureCube* pTextureCube, uint binding) override;
		void SetTextureArray(ITextureArray* pTextureArray, uint binding) override;

	private:
		void BindImageView(VkImageView view, VkImageLayout layout, uint binding);

		VulkanShader* _shader;
		VkDescriptorSet _textureSet;
		VkDescriptorSet _samplerSet;

		VkDescriptorSetLayout _textureSetLayout;
		VkDescriptorSetLayout _samplerSetLayout;

		Map<uint, uint> _textureBindingFixup;
		Map<uint, uint> _samplerBindingFixup;
	};

	class VulkanShader : public VulkanObject, public IShader
	{
	public:
		enum SharedShaderSets
		{
			ENGINE_BUFFERS = 0,
			ENGINE_TEXTURES,
			ENGINE_SAMPLERS,
			USER_TEXTURES,
			USER_SAMPLERS,

			SHARED_SETS_COUNT
		};

		static bool QuerySetAndBinding(ShaderResourceType type, uint inBinding, uint& outSet, uint& outBinding);
		static bool ResourceHasOwnSet(ShaderResourceType type, uint binding);

		VulkanShader();
		~VulkanShader();

		bool Create(IShaderCreateInfo& info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		VkPipelineLayout GetPipelineLayout() const;
		VkDescriptorSetLayout GetDescriptorSetLayout(uint set) const;

		inline VkDescriptorSet GetSharedBufferDescriptorSet() const { return _sharedBufferSet; }
		void GetSharedBufferDynamicOffsets(uint** ppOffsets, uint& numOffsets);

		//VkDescriptorSetLayoutBinding GetDescriptorSetLayoutBinding(DescriptorSet set, uint binding) const;

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

		Vector<VkVertexInputAttributeDescription> _inputAttribs;
		VkVertexInputBindingDescription _inputBinding;

		Vector<VkDescriptorSetLayout> _setLayouts;

		Map<VulkanUniformBuffer*, UniformBufferData> _uniformBuffersMap;

		//Map<VulkanUniformBuffer*, uint> _activeBuffersMap;
		//Map<uint, uint> _bindingToSortedIndexMap;
		//Vector<uint> _dynamicOffsetTable;

		VkPipelineLayout _layout;

		BindingData _previousBindings[MAX_BINDINGS_PER_DRAW];
		uint _prevBindingCount;

		BindingData _currentBindings[MAX_BINDINGS_PER_DRAW];
		uint _currBindingCount;

		VkDescriptorSet _sharedBufferSet;
		Vector<uint> _sharedBufferDynamicOffsets;

	};
}