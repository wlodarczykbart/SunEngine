#include <assert.h>

#include "ConfigFile.h"
#include "StringUtil.h"
#include "FileReader.h"

#include "VulkanCommandBuffer.h"
#include "VulkanRenderTarget.h"
#include "VulkanSampler.h"
#include "VulkanUniformBuffer.h"
#include "VulkanTexture.h"

#include "GraphicsContext.h"
#include "Sampler.h"

#include "VulkanShader.h"


namespace SunEngine
{
	VulkanShader::VulkanShader()
	{
		_layout = VK_NULL_HANDLE;
		_computePipeline = VK_NULL_HANDLE;
		_inputBinding = {};

		memset(_currentBindings, 0, sizeof(_currentBindings));
		memset(_previousBindings, 0, sizeof(_previousBindings));

		_prevBindingCount = 0;
		_currBindingCount = 0;

		_vertShader = VK_NULL_HANDLE;
		_fragShader = VK_NULL_HANDLE;
		_geomShader = VK_NULL_HANDLE;
		_compShader = VK_NULL_HANDLE;
	}


	VulkanShader::~VulkanShader()
	{
	}

	bool VulkanShader::Create(IShaderCreateInfo& info)
	{
		VkShaderModuleCreateInfo shaderInfo = {};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		if (info.vertexBinaries[SE_GFX_VULKAN].GetSize())
		{
			shaderInfo.pCode = (const uint*)info.vertexBinaries[SE_GFX_VULKAN].GetData();
			shaderInfo.codeSize = info.vertexBinaries[SE_GFX_VULKAN].GetSize();
			if (!_device->CreateShaderModule(shaderInfo, &_vertShader)) return false;
		}

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

		if (info.computeBinaries[SE_GFX_VULKAN].GetSize())
		{
			shaderInfo.pCode = (const uint*)info.computeBinaries[SE_GFX_VULKAN].GetData();
			shaderInfo.codeSize = info.computeBinaries[SE_GFX_VULKAN].GetSize();
			if (!_device->CreateShaderModule(shaderInfo, &_compShader)) return false;
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
			if (desc.stages & SS_COMPUTE)
				binding.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

			if (desc.type == SRT_TEXTURE)
				binding.descriptorType = desc.texture.readOnly ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			else if (desc.type == SRT_SAMPLER)
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			else if (desc.type == SRT_BUFFER)
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;

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

		//is a compute shader
		if (_compShader != VK_NULL_HANDLE)
		{
			VkPipelineShaderStageCreateInfo stageInfo = {};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			stageInfo.module = _compShader;
			stageInfo.pName = "main";

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.layout = _layout;
			pipelineInfo.stage = stageInfo;
			
			if (!_device->CreateComputePipeline(pipelineInfo, &_computePipeline)) return false;
		}

		return true;
	}

	bool VulkanShader::Destroy()
	{
		if (_vertShader != VK_NULL_HANDLE) _device->DestroyShaderModule(_vertShader); _vertShader = VK_NULL_HANDLE;
		if (_fragShader != VK_NULL_HANDLE) _device->DestroyShaderModule(_fragShader); _fragShader = VK_NULL_HANDLE;
		if (_geomShader != VK_NULL_HANDLE) _device->DestroyShaderModule(_geomShader); _geomShader = VK_NULL_HANDLE;
		if (_compShader != VK_NULL_HANDLE) _device->DestroyShaderModule(_compShader); _compShader = VK_NULL_HANDLE;
		if (_computePipeline != VK_NULL_HANDLE) _device->DestroyPipeline(_computePipeline); _computePipeline = VK_NULL_HANDLE;

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

		if (_computePipeline != VK_NULL_HANDLE)
		{
			VulkanCommandBuffer* vkCmd = static_cast<VulkanCommandBuffer*>(cmdBuffer);
			vkCmd->BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, _computePipeline, _layout);
		}
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

	bool VulkanShader::IsComputeShader() const
	{
		return _compShader != VK_NULL_HANDLE && _computePipeline != VK_NULL_HANDLE;
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
		_setNumber = createInfo.type;

		Vector<VkDescriptorSetLayoutBinding> layoutBindings;
		VkDescriptorSetLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		uint bufferCount = 0;
		_firstBuffer = UINT32_MAX;
		
		VulkanShader* pShader = static_cast<VulkanShader*>(createInfo.pShader);

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
			if (createInfo.resourceBindings[i].stages & SS_COMPUTE)
				binding.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

			if (createInfo.resourceBindings[i].type == SRT_TEXTURE)
			{
				if (createInfo.resourceBindings[i].texture.readOnly)
				{
					binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				}
				else
				{
					binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
					if (pShader->IsComputeShader())
					{
						// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
						VkImageMemoryBarrier imageMemoryBarrier = {};
						imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						// We won't be changing the layout of the image
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
						imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS , 0, VK_REMAINING_ARRAY_LAYERS };
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
						imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
						_computeBarriers[binding.binding] = imageMemoryBarrier;
					}
				}
			}
			else if (createInfo.resourceBindings[i].type == SRT_SAMPLER)
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			else if (createInfo.resourceBindings[i].type == SRT_BUFFER)
			{
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				_firstBuffer = min(_firstBuffer, binding.binding);
				bufferCount++;
			}

			_bindingMap[createInfo.resourceBindings[i].name].resource = createInfo.resourceBindings[i];
			layoutBindings.push_back(binding);
		}

		_dynamicOffsets.resize(bufferCount);
		memset(_dynamicOffsets.data(), 0, sizeof(uint) * _dynamicOffsets.size());

		info.bindingCount = layoutBindings.size();
		info.pBindings = layoutBindings.data();

		if (!_device->CreateDescriptorSetLayout(info, &_layout)) return false;

		_sets.resize(bufferCount > 0 ? VulkanDevice::BUFFERED_FRAME_COUNT : 1);
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

	bool VulkanShaderBindings::Destroy()
	{
		_device->DestroyDescriptorSetLayout(_layout);
		_bindingMap.clear();
		_dynamicOffsets.clear();
		_computeBarriers.clear();
		
		for (uint i = 0; i < _sets.size(); i++)
			_device->FreeDescriptorSet(_sets[i]);
		_sets.clear();
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
				while (!state->DynamicIndices[idx].first.empty() && idx < SE_ARR_SIZE(state->DynamicIndices))
				{
					auto& buffInfo = _bindingMap.at(state->DynamicIndices[idx].first);
					_dynamicOffsets[_firstBuffer - buffInfo.resource.binding[SE_GFX_VULKAN]] = static_cast<VulkanUniformBuffer*>(buffInfo.pObject)->GetAlignedSize() * state->DynamicIndices[idx].second;
					idx++;
				}
			}

			VulkanCommandBuffer* vkCmd = static_cast<VulkanCommandBuffer*>(pCmdBuffer);
			vkCmd->BindDescriptorSets(_setNumber, 1, &set, _dynamicOffsets.size(), _dynamicOffsets.data());

			if (_dynamicOffsets.size())
				memset(_dynamicOffsets.data(), 0x0, sizeof(uint) * _dynamicOffsets.size());
		}
	}																							 

	void VulkanShaderBindings::Unbind(ICommandBuffer * pCmdBuffer)
	{
		(void)pCmdBuffer;

		VulkanCommandBuffer* vkCmd = static_cast<VulkanCommandBuffer*>(pCmdBuffer);
		for (auto iter = _computeBarriers.begin(); iter != _computeBarriers.end(); ++iter)
			vkCmd->PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, (*iter).second);
	}

	void VulkanShaderBindings::SetUniformBuffer(IUniformBuffer* pBuffer, const String& name)
	{
		VulkanUniformBuffer* pVulkanBuffer = static_cast<VulkanUniformBuffer*>(pBuffer);
		ResourceBindData& data = _bindingMap.at(name);
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
			set.dstBinding = data.resource.binding[SE_GFX_VULKAN];
			set.pBufferInfo = &info;
			set.descriptorCount = 1;

			_device->UpdateDescriptorSets(&set, 1);
		}
		data.pObject = pVulkanBuffer;
	}

	void VulkanShaderBindings::SetTexture(ITexture * pTexture, const String& name)
	{
		VulkanTexture* vkTexture = static_cast<VulkanTexture*>(pTexture);
		ResourceBindData& data = _bindingMap.at(name);

		bool cubemapToArray = 
			(vkTexture->_viewInfo.viewType == VK_IMAGE_VIEW_TYPE_CUBE || vkTexture->_viewInfo.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) 
			&& data.resource.texture.dimension == SRD_TEXTURE2DARRAY;

		VkDescriptorImageInfo info = {};
		info.imageView = cubemapToArray ? vkTexture->_cubeToArrayView : vkTexture->_view;
		info.imageLayout = vkTexture->_layout;

		for (uint i = 0; i < _sets.size(); i++)
		{
			VkWriteDescriptorSet set = {};
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.dstSet = _sets[i];
			set.descriptorType = data.resource.texture.readOnly ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			set.dstBinding = data.resource.binding[SE_GFX_VULKAN];
			set.pImageInfo = &info;
			set.descriptorCount = 1;

			_device->UpdateDescriptorSets(&set, 1);
		}
		data.pObject = vkTexture;

		auto barrier = _computeBarriers.find(data.resource.binding[SE_GFX_VULKAN]);
		if (barrier != _computeBarriers.end())
			(*barrier).second.image = vkTexture->_image;
	}

	void VulkanShaderBindings::SetSampler(ISampler * pSampler, const String& name)
	{
		VkDescriptorImageInfo info = {};
		info.sampler = static_cast<VulkanSampler*>(pSampler)->_sampler;

		ResourceBindData& data = _bindingMap.at(name);
		for (uint i = 0; i < _sets.size(); i++)
		{
			VkWriteDescriptorSet set = {};
			set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			set.dstSet = _sets[i];
			set.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			set.dstBinding = data.resource.binding[SE_GFX_VULKAN];
			set.pImageInfo = &info;
			set.descriptorCount = 1;

			_device->UpdateDescriptorSets(&set, 1);
		}
		data.pObject = static_cast<VulkanSampler*>(pSampler);
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