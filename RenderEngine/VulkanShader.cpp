#include <assert.h>

#include "ConfigFile.h"
#include "StringUtil.h"
#include "FileReader.h"

#include "VulkanCommandBuffer.h"
#include "VulkanRenderTarget.h"
#include "VulkanSampler.h"
#include "VulkanUniformBuffer.h"
#include "VulkanTexture.h"
#include "VulkanTextureCube.h"

#include "VulkanShader.h"


namespace SunEngine
{
	class VulkanResourceFixup
	{
	public:
		static VulkanResourceFixup& Get()
		{
			static VulkanResourceFixup instance;
			return instance;
		}

		bool QuerySetAndBinding(ShaderResourceType type, uint inBinding, uint& outSet, uint& outBinding)
		{
			auto found = _resFixup.find(type);
			if (found != _resFixup.end())
			{
				auto foundInner = (*found).second.find(inBinding);
				if (foundInner != (*found).second.end()) 
				{

					outSet = (*foundInner).second.first;
					outBinding = (*foundInner).second.second;
					return true;
				}
			}

			if (type == SRT_TEXTURE)
			{
				outSet = VulkanShader::USER_TEXTURES;
				outBinding = inBinding - STL_COUNT;
				return true;
			}

			if (type == SRT_SAMPLER)
			{
				outSet = VulkanShader::USER_SAMPLERS;
				outBinding = inBinding - STL_COUNT;
				return true;
			}

			return false;
		}

	private:
		VulkanResourceFixup()
		{
			Vector<uint> UniqueUniformBuffers =
			{
				SBL_OBJECT_BUFFER,
				SBL_MATERIAL,
				SBL_TEXTURE_TRANSFORM,
				SBL_SKINNED_BONES,
			};

			uint uniqueSet = VulkanShader::SHARED_SETS_COUNT;

			for (uint i = 0; i < UniqueUniformBuffers.size(); i++)
			{
				_resFixup[SRT_CONST_BUFFER][UniqueUniformBuffers[i]] = { uniqueSet++, 0 };
			}

			for (uint i = 0; i < STL_COUNT; i++)
			{
				_resFixup[SRT_TEXTURE][i] = { VulkanShader::ENGINE_TEXTURES, i };
				_resFixup[SRT_SAMPLER][i] = { VulkanShader::ENGINE_SAMPLERS, i };
			}

			Map<uint, Pair<uint, uint>>& bufferMap = _resFixup.at(SRT_CONST_BUFFER);
			uint sharedBufferBinding = 0;
			for (uint i = 0; i < SBL_COUNT; i++)
			{
				if (bufferMap.find(i) == bufferMap.end())
				{
					bufferMap[i] = { VulkanShader::ENGINE_BUFFERS, sharedBufferBinding++ };
				}
			}
		}

		Map<ShaderResourceType, Map<uint, Pair<uint, uint>>> _resFixup;
	};

	bool VulkanShader::QuerySetAndBinding(ShaderResourceType type, uint inBinding, uint& outSet, uint& outBinding)
	{
		return VulkanResourceFixup::Get().QuerySetAndBinding(type, inBinding, outSet, outBinding);
	}

	bool VulkanShader::ResourceHasOwnSet(ShaderResourceType type, uint binding)
	{
		return false;
	}

	VulkanShader::VulkanShader()
	{
		_layout = VK_NULL_HANDLE;
		_inputBinding = {};

		memset(_currentBindings, 0, sizeof(_currentBindings));
		memset(_previousBindings, 0, sizeof(_previousBindings));

		_prevBindingCount = 0;
		_currBindingCount = 0;

		_vertShader = VK_NULL_HANDLE;
		_fragShader = VK_NULL_HANDLE;
		_geomShader = VK_NULL_HANDLE;
	}


	VulkanShader::~VulkanShader()
	{
	}

	bool VulkanShader::Create(IShaderCreateInfo& info)
	{
		FileReader rdr;
		VkShaderModuleCreateInfo shaderInfo = {};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		MemBuffer vertBuffer;
		rdr.Open(info.vertexShaderFilename.data());
		rdr.ReadAll(vertBuffer);
		rdr.Close();
		shaderInfo.pCode = (const uint*)vertBuffer.GetData();
		shaderInfo.codeSize = vertBuffer.GetSize();
		if (!_device->CreateShaderModule(shaderInfo, &_vertShader)) return false;

		if (info.pixelShaderFilename.length())
		{
			MemBuffer pixelBuffer;
			rdr.Open(info.pixelShaderFilename.data());
			rdr.ReadAll(pixelBuffer);
			rdr.Close();
			shaderInfo.pCode = (const uint*)pixelBuffer.GetData();
			shaderInfo.codeSize = pixelBuffer.GetSize();
			if (!_device->CreateShaderModule(shaderInfo, &_fragShader)) return false;
		}

		if (info.geometryShaderFilename.length())
		{
			MemBuffer geomBuffer;
			rdr.Open(info.geometryShaderFilename.data());
			rdr.ReadAll(geomBuffer);
			rdr.Close();
			shaderInfo.pCode = (const uint*)geomBuffer.GetData();
			shaderInfo.codeSize = geomBuffer.GetSize();
			if (!_device->CreateShaderModule(shaderInfo, &_geomShader)) return false;
		}

		Vector<APIVertexElement> sortedElements = info.vertexElements;
		qsort(sortedElements.data(), sortedElements.size(), sizeof(APIVertexElement), [](const void* left, const void* right) -> int {
			return ((APIVertexElement*)left)->offset < ((APIVertexElement*)right)->offset ? -1 : 1;
		});

		_inputAttribs.resize(sortedElements.size());
		for (uint i = 0; i < sortedElements.size(); i++)
		{
			const APIVertexElement &attrib = sortedElements[i];
			_inputAttribs[i].binding = 0;
			_inputAttribs[i].location = i;
			_inputAttribs[i].offset = attrib.offset;

			switch (attrib.format)
			{
			case APIVertexInputFormat::VIF_FLOAT2:
				_inputAttribs[i].format = VK_FORMAT_R32G32_SFLOAT;
				break;
			case APIVertexInputFormat::VIF_FLOAT3:
				_inputAttribs[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				break;
			case APIVertexInputFormat::VIF_FLOAT4:
				_inputAttribs[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			default:
				break;
			}

			_inputBinding.stride += attrib.size;
		}


		Vector<VkDescriptorSetLayoutBinding> vDescriptorSetLayoutBindings[32];
		uint maxSet = 0;

		for (uint i = 0; i < info.constBuffers.size(); i++)
		{
			IShaderResource& desc = info.constBuffers[i];

			uint setNum, bindingNum;
			if (!QuerySetAndBinding(SRT_CONST_BUFFER, desc.binding, setNum, bindingNum))
				return false;

			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = bindingNum;
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

			if (desc.stages & SS_VERTEX)
				binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (desc.stages & SS_PIXEL)
				binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (desc.stages & SS_GEOMETRY)
				binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			//_uniformBuffersMap[desc.binding] = static_cast<VulkanUniformBuffer*>(info.constBuffers[i].second);
			vDescriptorSetLayoutBindings[setNum].push_back(binding);
			if (setNum > maxSet)
				maxSet = setNum;
		}

		//Vector<VkDescriptorBindingFlagsEXT> texBindingFlags;
		for (uint i = 0; i < info.textures.size(); i++)
		{
			IShaderResource& desc = info.textures[i];

			uint setNum, bindingNum;
			if (!QuerySetAndBinding(SRT_TEXTURE, desc.binding, setNum, bindingNum))
				return false;

			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = bindingNum;
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

			if (desc.stages & SS_VERTEX)
				binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (desc.stages & SS_PIXEL)
				binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (desc.stages & SS_GEOMETRY)
				binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			vDescriptorSetLayoutBindings[setNum].push_back(binding);

			if (setNum > maxSet)
				maxSet = setNum;

			//VkDescriptorBindingFlagBitsEXT texFlags =
			//	VkDescriptorBindingFlagBitsEXT::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
			//texBindingFlags.push_back(texFlags);
		}

		//VkDescriptorSetLayoutBindingFlagsCreateInfoEXT texBindingFlagsCreateInfo = {};
		//texBindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		//texBindingFlagsCreateInfo.bindingCount = texBindingFlags.size();
		//texBindingFlagsCreateInfo.pBindingFlags = texBindingFlags.data();

		//if (_device->ContainsExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME))
		//{
		//	setInfos[TEXTURE_SET].pNext = &texBindingFlagsCreateInfo;
		//	setInfos[TEXTURE_SET].flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		//}
		

		//Vector<VkDescriptorBindingFlagsEXT> samplerBindingFlags;
		for (uint i = 0; i < info.samplers.size(); i++)
		{
			IShaderResource& desc = info.samplers[i];

			uint setNum, bindingNum;
			if (!QuerySetAndBinding(SRT_SAMPLER, desc.binding, setNum, bindingNum))
				return false;

			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = bindingNum;
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

			if (desc.stages & SS_VERTEX)
				binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (desc.stages & SS_PIXEL)
				binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (desc.stages & SS_GEOMETRY)
				binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			vDescriptorSetLayoutBindings[setNum].push_back(binding);

			if (setNum > maxSet)
				maxSet = setNum;

			//VkDescriptorBindingFlagBitsEXT samplerFlags =
			//	VkDescriptorBindingFlagBitsEXT::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
			//samplerBindingFlags.push_back(samplerFlags);

		}

		//VkDescriptorSetLayoutBindingFlagsCreateInfoEXT samplerBindingFlagsCreateInfo = {};
		//samplerBindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
		//samplerBindingFlagsCreateInfo.bindingCount = samplerBindingFlags.size();
		//samplerBindingFlagsCreateInfo.pBindingFlags = samplerBindingFlags.data();

		//if (_device->ContainsExtension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME))
		//{
		//	setInfos[SAMPLER_SET].pNext = &samplerBindingFlagsCreateInfo;
		//	setInfos[SAMPLER_SET].flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		//}


		maxSet = maxSet + 1;
		_setLayouts.resize(maxSet);
		for (uint i = 0; i < maxSet; i++)
		{
			VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
			setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			setLayoutInfo.bindingCount = vDescriptorSetLayoutBindings[i].size();
			setLayoutInfo.pBindings = vDescriptorSetLayoutBindings[i].data();

			if(!_device->CreateDescriptorSetLayout(setLayoutInfo, &_setLayouts[i]))
				return false;
		}


		//Not supporting push constants 
		VkPipelineLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = _setLayouts.size();
		layoutInfo.pSetLayouts = _setLayouts.data();
		if (!_device->CreatePipelineLayout(layoutInfo, &_layout)) return false;


		VkDescriptorSetAllocateInfo setInfo = {};
		setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setInfo.descriptorSetCount = 1;
		setInfo.pSetLayouts = &_setLayouts[ENGINE_BUFFERS];

		if (!_device->AllocateDescriptorSets(setInfo, &_sharedBufferSet))
			return false;

		_sharedBufferDynamicOffsets.resize(vDescriptorSetLayoutBindings[ENGINE_BUFFERS].size());
		memset(_sharedBufferDynamicOffsets.data(), 0, sizeof(uint)* _sharedBufferDynamicOffsets.size());

		//for (uint i = 0; i < DESCRIPTOR_SET_COUNT; i++)
		//{
		//	for (uint j = 0; j < vdescriptorSetLayoutBindings[i].size(); j++)
		//	{
		//		_descriptorSetLayoutBindings[i][vdescriptorSetLayoutBindings[i][j].binding] = vdescriptorSetLayoutBindings[i][j];
		//	}
		//}

		return true;
	}

	bool VulkanShader::Destroy()
	{
		if (_vertShader != VK_NULL_HANDLE) _device->DestroyShaderModule(_vertShader); _vertShader = VK_NULL_HANDLE;
		if (_fragShader != VK_NULL_HANDLE) _device->DestroyShaderModule(_fragShader); _fragShader = VK_NULL_HANDLE;
		if (_geomShader != VK_NULL_HANDLE) _device->DestroyShaderModule(_geomShader); _geomShader = VK_NULL_HANDLE;

		for (uint i = 0; i < _setLayouts.size(); i++)
		{
			_device->DestroyDescriptorSetLayout(_setLayouts[i]);
		}
		
		_device->DestroyPipelineLayout(_layout);

		_inputAttribs.clear();
		_uniformBuffersMap.clear();

		_layout = VK_NULL_HANDLE;
		_inputBinding = {};
		_setLayouts.clear(); //can these be destroyed?

		return true;
	}

	void VulkanShader::Bind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	void VulkanShader::Unbind(ICommandBuffer * cmdBuffer)
	{
		//Map<uint, VulkanUniformBuffer*>::iterator uboIter = _uniformBuffersMap.begin();
		//while (uboIter != _uniformBuffersMap.end())
		//{
		//	(*uboIter).second->_activeShader = 0;
		//	(*uboIter).second->_activeBinding = 0;
		//	uint tableIndex = (*uboIter).first;
		//	_dynamicOffsetTable[tableIndex] = (*uboIter).second->_prevOffset;
		//	uboIter++;
		//}

		(void)cmdBuffer;
	}

	VkPipelineLayout VulkanShader::GetPipelineLayout() const
	{
		return _layout;
	}

	VkDescriptorSetLayout VulkanShader::GetDescriptorSetLayout(uint set) const
	{
		return _setLayouts.at(set);
	}

	void VulkanShader::GetSharedBufferDynamicOffsets(uint** ppOffsets, uint& numOffsets)
	{
		*ppOffsets = _sharedBufferDynamicOffsets.size() ? _sharedBufferDynamicOffsets.data() : 0;
		numOffsets = _sharedBufferDynamicOffsets.size();
	}

	int VulkanShader::SortBindings(const void * lhs, const void * rhs)
	{
		int ileft = *((int*)lhs);
		int iright = *((int*)rhs);

		if (ileft < iright)
			return -1;
		else
			return 1;
	}

	//bool VulkanShader::GetSetIndex(VkDescriptorType type, uint& set)
	//{
	//	Map<VkDescriptorType, uint>::iterator setIt = _setTypeMap.find(type);
	//	if (setIt != _setTypeMap.end())
	//	{
	//		set = (*setIt).second;
	//		return true;
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	VulkanShaderBindings::VulkanShaderBindings()
	{
		_textureSet = VK_NULL_HANDLE;
		_samplerSet = VK_NULL_HANDLE;
		_textureSetLayout = VK_NULL_HANDLE;
		_samplerSetLayout = VK_NULL_HANDLE;
	}

	VulkanShaderBindings::~VulkanShaderBindings()
	{
	}

	//bool VulkanShaderBindings::Create(IShader * pShader)
	//{
	//	_shader = static_cast<VulkanShader*>(pShader);

	//	VkDescriptorSetLayout textureSet = _shader->GetDescriptorSetLayout(VulkanShader::TEXTURE_SET);
	//	VkDescriptorSetAllocateInfo texAllocInfo = {};
	//	texAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	//	texAllocInfo.descriptorSetCount = 1;
	//	texAllocInfo.pSetLayouts = &textureSet;
	//	if (!_device->AllocateDescriptorSets(texAllocInfo, &_textureSet)) return false;

	//	VkDescriptorSetLayout samplerSet = _shader->GetDescriptorSetLayout(VulkanShader::SAMPLER_SET);
	//	VkDescriptorSetAllocateInfo samplerAllocInfo = {};
	//	samplerAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	//	samplerAllocInfo.descriptorSetCount = 1;
	//	samplerAllocInfo.pSetLayouts = &samplerSet;
	//	if (!_device->AllocateDescriptorSets(samplerAllocInfo, &_samplerSet)) return false;

	//	return true;
	//}

	bool VulkanShaderBindings::Create(IShader* pShader, const Vector<IShaderResource>& textureBindings, const Vector<IShaderResource>& samplerBindings)
	{
		_shader = static_cast<VulkanShader*>(pShader);
		//TODO: clean up old handles

		Vector<VkDescriptorSetLayoutBinding> textureInfos, samplerInfos;
		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		textureInfos.resize(textureBindings.size());
		info.bindingCount = textureBindings.size();
		info.pBindings = textureInfos.data();
		for (uint i = 0; i < textureInfos.size(); i++)
		{
			uint setNum, bindingNum;
			VulkanShader::QuerySetAndBinding(SRT_TEXTURE, textureBindings[i].binding, setNum, bindingNum);
			assert(setNum == VulkanShader::USER_TEXTURES);

			textureInfos[i] = {};
			textureInfos[i].binding = bindingNum;
			textureInfos[i].descriptorCount = 1;
			textureInfos[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

			_textureBindingFixup[textureBindings[i].binding] = bindingNum;

			uint stageFlags = 0;
			if (textureBindings[i].stages & SS_VERTEX)
				stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (textureBindings[i].stages & SS_PIXEL)
				stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (textureBindings[i].stages & SS_GEOMETRY)
				stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			textureInfos[i].stageFlags = stageFlags;
		}

		if (!_device->CreateDescriptorSetLayout(info, &_textureSetLayout)) return false;

		samplerInfos.resize(samplerBindings.size());
		info.bindingCount = samplerBindings.size();
		info.pBindings = samplerInfos.data();
		for (uint i = 0; i < samplerInfos.size(); i++)
		{
			uint setNum, bindingNum;
			VulkanShader::QuerySetAndBinding(SRT_SAMPLER, samplerBindings[i].binding, setNum, bindingNum);
			assert(setNum == VulkanShader::USER_SAMPLERS);

			samplerInfos[i] = {};
			samplerInfos[i].binding = bindingNum;
			samplerInfos[i].descriptorCount = 1;
			samplerInfos[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

			_samplerBindingFixup[samplerBindings[i].binding] = bindingNum;

			uint stageFlags = 0;
			if (samplerBindings[i].stages & SS_VERTEX)
				stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (samplerBindings[i].stages & SS_PIXEL)
				stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (samplerBindings[i].stages & SS_GEOMETRY)
				stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			samplerInfos[i].stageFlags = stageFlags;
		}

		if (!_device->CreateDescriptorSetLayout(info, &_samplerSetLayout)) return false;

		VkDescriptorSetAllocateInfo texAllocInfo = {};
		texAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		texAllocInfo.descriptorSetCount = 1;
		texAllocInfo.pSetLayouts = &_textureSetLayout;
		if (!_device->AllocateDescriptorSets(texAllocInfo, &_textureSet)) return false;

		VkDescriptorSetAllocateInfo samplerAllocInfo = {};
		samplerAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		samplerAllocInfo.descriptorSetCount = 1;
		samplerAllocInfo.pSetLayouts = &_samplerSetLayout;
		if (!_device->AllocateDescriptorSets(samplerAllocInfo, &_samplerSet)) return false;

		return true;
	}

	void VulkanShaderBindings::Bind(ICommandBuffer * pCmdBuffer)
	{
		VulkanCommandBuffer* vkCmd = static_cast<VulkanCommandBuffer*>(pCmdBuffer);
		vkCmd->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _shader->GetPipelineLayout(), VulkanShader::USER_TEXTURES, 1, &_textureSet, 0, NULL);
		vkCmd->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _shader->GetPipelineLayout(), VulkanShader::USER_SAMPLERS, 1, &_samplerSet, 0, NULL);
	}																							 

	void VulkanShaderBindings::Unbind(ICommandBuffer * pCmdBuffer)
	{
		(void)pCmdBuffer;
	}

	void VulkanShaderBindings::SetTexture(ITexture * pTexture, const uint binding)
	{
		BindImageView(static_cast<VulkanTexture*>(pTexture)->_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, binding);
	}

	void VulkanShaderBindings::SetSampler(ISampler * pSampler, const uint binding)
	{
		VkDescriptorImageInfo info = {};
		info.sampler = static_cast<VulkanSampler*>(pSampler)->_sampler;

		VkWriteDescriptorSet set = {};
		set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set.dstSet = _samplerSet;
		set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		set.dstBinding = _samplerBindingFixup.at(binding);
		set.pImageInfo = &info;
		set.descriptorCount = 1;

		_device->UpdateDescriptorSets(&set, 1);
	}

	void VulkanShaderBindings::SetTextureCube(ITextureCube * pTextureCube, const uint binding)
	{
		BindImageView(static_cast<VulkanTextureCube*>(pTextureCube)->_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, binding);
	}

	void VulkanShaderBindings::SetTextureArray(ITextureArray * pTextureArray, uint binding)
	{
		(void)pTextureArray;
		(void)binding;
		//TODO:!!!!!!
	}

	void VulkanShaderBindings::BindImageView(VkImageView view, VkImageLayout layout, const uint binding)
	{
		VkDescriptorImageInfo info = {};
		info.imageView = view;
		info.imageLayout = layout;

		VkWriteDescriptorSet set = {};
		set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		set.dstSet = _textureSet;
		set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		set.dstBinding = _textureBindingFixup.at(binding);
		set.pImageInfo = &info;
		set.descriptorCount = 1;

		_device->UpdateDescriptorSets(&set, 1);

	}

	//bool VulkanShader::ObjectIsCurrentlyBound(const uint set, const uint binding, uint64_t apiHandle)
	//{
	//	Vector<uint64_t>& bindings = _setBindings.at(set);
	//	
	//	if (bindings[binding] == apiHandle)
	//	{
	//		return true;
	//	}
	//	else
	//	{
	//		bindings[binding] = apiHandle;
	//		return false;
	//	}
	//}
}