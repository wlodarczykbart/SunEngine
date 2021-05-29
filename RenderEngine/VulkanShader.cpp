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

#include "GraphicsContext.h"
#include "Sampler.h"

#include "VulkanShader.h"


namespace SunEngine
{
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
		VkShaderModuleCreateInfo shaderInfo = {};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;


		shaderInfo.pCode = (const uint*)info.vertexBinaries[SE_GFX_VULKAN].GetData();
		shaderInfo.codeSize = info.vertexBinaries[SE_GFX_VULKAN].GetSize();
		if (!_device->CreateShaderModule(shaderInfo, &_vertShader)) return false;

		if (info.pixelBinaries[SE_GFX_VULKAN].GetSize())
		{
			shaderInfo.pCode = (const uint*)info.pixelBinaries[SE_GFX_VULKAN].GetData();
			shaderInfo.codeSize = info.pixelBinaries[SE_GFX_VULKAN].GetSize();
			if (!_device->CreateShaderModule(shaderInfo, &_fragShader)) return false;
		}

		if (info.geometryBinaries[SE_GFX_VULKAN].GetSize())
		{
			shaderInfo.pCode = (const uint*)info.geometryBinaries[SE_GFX_VULKAN].GetData();
			shaderInfo.codeSize = info.geometryBinaries[SE_GFX_VULKAN].GetSize();
			if (!_device->CreateShaderModule(shaderInfo, &_geomShader)) return false;
		}

		Vector<IVertexElement> sortedElements = info.vertexElements;
		qsort(sortedElements.data(), sortedElements.size(), sizeof(IVertexElement), [](const void* left, const void* right) -> int {
			return ((IVertexElement*)left)->offset < ((IVertexElement*)right)->offset ? -1 : 1;
		});

		_inputAttribs.resize(sortedElements.size());
		for (uint i = 0; i < sortedElements.size(); i++)
		{
			const IVertexElement&attrib = sortedElements[i];
			_inputAttribs[i].binding = 0;
			_inputAttribs[i].location = i;
			_inputAttribs[i].offset = attrib.offset;

			switch (attrib.format)
			{
			case IVertexInputFormat::VIF_FLOAT2:
				_inputAttribs[i].format = VK_FORMAT_R32G32_SFLOAT;
				break;
			case IVertexInputFormat::VIF_FLOAT3:
				_inputAttribs[i].format = VK_FORMAT_R32G32B32_SFLOAT;
				break;
			case IVertexInputFormat::VIF_FLOAT4:
				_inputAttribs[i].format = VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			default:
				break;
			}

			_inputBinding.stride += attrib.size;
		}


		Vector<VkDescriptorSetLayoutBinding> vDescriptorSetLayoutBindings[32];
		uint maxSet = 0;

		for (auto iter = info.buffers.begin(); iter != info.buffers.end(); ++iter)
		{
			IShaderBuffer& desc = (*iter).second;

			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = desc.binding[SE_GFX_VULKAN];
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

			if (desc.stages & SS_VERTEX)
				binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (desc.stages & SS_PIXEL)
				binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (desc.stages & SS_GEOMETRY)
				binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			//_uniformBuffersMap[desc.binding] = static_cast<VulkanUniformBuffer*>(info.constBuffers[i].second);
			vDescriptorSetLayoutBindings[desc.bindType].push_back(binding);
			if (desc.bindType > maxSet)
				maxSet = desc.bindType;
		}

		//Vector<VkDescriptorBindingFlagsEXT> texBindingFlags;
		for (auto iter = info.resources.begin(); iter != info.resources.end(); ++iter)
		{
			IShaderResource& desc = (*iter).second;

			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = desc.binding[SE_GFX_VULKAN];
			binding.descriptorCount = 1;

			if (desc.stages & SS_VERTEX)
				binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (desc.stages & SS_PIXEL)
				binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (desc.stages & SS_GEOMETRY)
				binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			if(desc.type == SRT_TEXTURE)
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			else if(desc.type == SRT_SAMPLER)
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

			vDescriptorSetLayoutBindings[desc.bindType].push_back(binding);

			if (desc.bindType > maxSet)
				maxSet = desc.bindType;

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
		

		////Vector<VkDescriptorBindingFlagsEXT> samplerBindingFlags;
		//for (uint i = 0; i < info.samplers.size(); i++)
		//{
		//	IShaderResource& desc = info.samplers[i];

		//	uint setNum = 0, bindingNum = 0;
		//	//if (!QuerySetAndBinding(SRT_CONST_BUFFER, desc.binding, setNum, bindingNum))
		//	//	return false;

		//	VkDescriptorSetLayoutBinding binding = {};
		//	binding.binding = bindingNum;
		//	binding.descriptorCount = 1;
		//	binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

		//	if (desc.stages & SS_VERTEX)
		//		binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		//	if (desc.stages & SS_PIXEL)
		//		binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		//	if (desc.stages & SS_GEOMETRY)
		//		binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

		//	vDescriptorSetLayoutBindings[setNum].push_back(binding);

		//	if (setNum > maxSet)
		//		maxSet = setNum;

		//	//VkDescriptorBindingFlagBitsEXT samplerFlags =
		//	//	VkDescriptorBindingFlagBitsEXT::VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
		//	//samplerBindingFlags.push_back(samplerFlags);

		//}

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

		_layout = VK_NULL_HANDLE;
		_inputBinding = {};
		_setLayouts.clear(); //can these be destroyed?

		return true;
	}

	void VulkanShader::Bind(ICommandBuffer* cmdBuffer, IBindState*)
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
		_setNumber = 0;
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

	bool VulkanShaderBindings::Create(const IShaderBindingCreateInfo& createInfo)
	{
		_shader = static_cast<VulkanShader*>(createInfo.pShader);
		_setNumber = createInfo.type;

		Vector<VkDescriptorSetLayoutBinding> layoutBindings;
		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		uint maxBufferBinding = 0;
		for (uint i = 0; i < createInfo.bufferBindings.size(); i++)
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = createInfo.bufferBindings[i].binding[SE_GFX_VULKAN];
			binding.descriptorCount = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

			if (createInfo.bufferBindings[i].stages & SS_VERTEX)
				binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (createInfo.bufferBindings[i].stages & SS_PIXEL)
				binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (createInfo.bufferBindings[i].stages & SS_GEOMETRY)
				binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			_bindingMap[createInfo.bufferBindings[i].name].first = binding.binding;
			layoutBindings.push_back(binding);

			if (binding.binding+1 > maxBufferBinding)
				maxBufferBinding = binding.binding+1;
		}

		_dynamicOffsets.resize(maxBufferBinding);
		memset(_dynamicOffsets.data(), 0, sizeof(uint) * _dynamicOffsets.size());

		for (uint i = 0; i < createInfo.resourceBindings.size(); i++)
		{
			VkDescriptorSetLayoutBinding binding = {};
			binding.binding = createInfo.resourceBindings[i].binding[SE_GFX_VULKAN];
			binding.descriptorCount = 1;

			if (createInfo.resourceBindings[i].stages & SS_VERTEX)
				binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
			if (createInfo.resourceBindings[i].stages & SS_PIXEL)
				binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			if (createInfo.resourceBindings[i].stages & SS_GEOMETRY)
				binding.stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			if (createInfo.resourceBindings[i].type == SRT_TEXTURE)
			{
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			}
			else if (createInfo.resourceBindings[i].type == SRT_SAMPLER)
			{
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			}

			_bindingMap[createInfo.resourceBindings[i].name].first = binding.binding;
			layoutBindings.push_back(binding);
		}

		info.bindingCount = layoutBindings.size();
		info.pBindings = layoutBindings.data();

		if (!_device->CreateDescriptorSetLayout(info, &_layout)) return false;

		_sets.resize(createInfo.bufferBindings.size() ? VulkanDevice::BUFFERED_FRAME_COUNT : 1);
		Vector<VkDescriptorSetLayout> layouts(_sets.size());
		for (uint i = 0; i < _sets.size(); i++)
			layouts[i] = _layout;

		VkDescriptorSetAllocateInfo setAllocInfo = {};
		setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		setAllocInfo.descriptorSetCount = _sets.size();
		setAllocInfo.pSetLayouts = layouts.data();
		if (!_device->AllocateDescriptorSets(setAllocInfo, _sets.data())) return false;

		return true;
	}

	void VulkanShaderBindings::Bind(ICommandBuffer * pCmdBuffer, IBindState* pBindState)
	{
		if (_sets.size())
		{
			VkDescriptorSet set = GetCurrentSet();
			if (pBindState && pBindState->GetType() == IOBT_SHADER_BINDINGS)
			{
				IShaderBindingsBindState* state = static_cast<IShaderBindingsBindState*>(pBindState);
				uint idx = 0;
				while (!state->DynamicIndices[idx].first.empty() && idx < ARRAYSIZE(state->DynamicIndices))
				{
					auto& buffInfo = _bindingMap.at(state->DynamicIndices[idx].first);

					_dynamicOffsets[buffInfo.first] = static_cast<VulkanUniformBuffer*>(buffInfo.second)->GetAlignedSize() * state->DynamicIndices[idx].second;
					idx++;
				}
			}

			VulkanCommandBuffer* vkCmd = static_cast<VulkanCommandBuffer*>(pCmdBuffer);
			vkCmd->BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, _shader->GetPipelineLayout(), _setNumber, 1, &set, _dynamicOffsets.size(), _dynamicOffsets.data());

			if (_dynamicOffsets.size())
				memset(_dynamicOffsets.data(), 0x0, sizeof(uint) * _dynamicOffsets.size());
		}
	}																							 

	void VulkanShaderBindings::Unbind(ICommandBuffer * pCmdBuffer)
	{
		(void)pCmdBuffer;
	}

	void VulkanShaderBindings::SetUniformBuffer(IUniformBuffer* pBuffer, const String& name)
	{
		VulkanUniformBuffer* pVulkanBuffer = static_cast<VulkanUniformBuffer*>(pBuffer);
		for (uint i = 0; i < _sets.size(); i++)
		{
			VkDescriptorBufferInfo info = {};
			uint bufferIndex = i % pVulkanBuffer->_buffers.size();
			info.buffer = pVulkanBuffer->_buffers[bufferIndex].buffer;
			info.range = pVulkanBuffer->_size;

			VkWriteDescriptorSet set = {};
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.dstSet = _sets[i];
			set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			set.dstBinding = _bindingMap.at(name).first;
			set.pBufferInfo = &info;
			set.descriptorCount = 1;

			_device->UpdateDescriptorSets(&set, 1);
		}
		_bindingMap.at(name).second = pVulkanBuffer;
	}

	void VulkanShaderBindings::SetTexture(ITexture * pTexture, const String& name)
	{
		VulkanTexture* vkTexture = static_cast<VulkanTexture*>(pTexture);

		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		switch (vkTexture->GetFormat())
		{
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT:
			layout = VulkanRenderTarget::GetDepthFinalLayout();
			break;
		default:
			layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;
		}

		BindImageView(vkTexture->_view, layout, _bindingMap.at(name).first);
		_bindingMap.at(name).second = vkTexture;
	}

	void VulkanShaderBindings::SetSampler(ISampler * pSampler, const String& name)
	{
		VkDescriptorImageInfo info = {};
		info.sampler = static_cast<VulkanSampler*>(pSampler)->_sampler;

		for (uint i = 0; i < _sets.size(); i++)
		{
			VkWriteDescriptorSet set = {};
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.dstSet = _sets[i];
			set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			set.dstBinding = _bindingMap.at(name).first;
			set.pImageInfo = &info;
			set.descriptorCount = 1;

			_device->UpdateDescriptorSets(&set, 1);
		}
		_bindingMap.at(name).second = static_cast<VulkanSampler*>(pSampler);
	}

	void VulkanShaderBindings::SetTextureCube(ITextureCube * pTextureCube, const String& name)
	{
		BindImageView(static_cast<VulkanTextureCube*>(pTextureCube)->_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _bindingMap.at(name).first);
		_bindingMap.at(name).second = static_cast<VulkanTextureCube*>(pTextureCube);
	}

	void VulkanShaderBindings::SetTextureArray(ITextureArray * pTextureArray, const String& name)
	{
		(void)pTextureArray;
		(void)name;
		//TODO:!!!!!!
	}

	void VulkanShaderBindings::BindImageView(VkImageView view, VkImageLayout layout, const uint binding)
	{
		VkDescriptorImageInfo info = {};
		info.imageView = view;
		info.imageLayout = layout;

		for (uint i = 0; i < _sets.size(); i++)
		{
			VkWriteDescriptorSet set = {};
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.dstSet = _sets[i];
			set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			set.dstBinding = binding;
			set.pImageInfo = &info;
			set.descriptorCount = 1;

			_device->UpdateDescriptorSets(&set, 1);
		}
	}

	VkDescriptorSet VulkanShaderBindings::GetCurrentSet() const
	{
		return _sets.size() == 1 ? _sets.front() : _sets[_device->GetBufferedFrameNumber()];
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