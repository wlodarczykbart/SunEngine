#include "IShader.h"
#include "IUniformBuffer.h"
#include "IShaderBindings.h"

#include "ConfigFile.h"
#include "FileReader.h"
#include "StringUtil.h"

#include "RenderTarget.h"
#include "Sampler.h"
#include "UniformBuffer.h"
#include "BaseTexture.h"
#include "BaseTextureCube.h"
#include "BaseTextureArray.h"

#include "GraphicsContext.h"

#include "IShader.h"
#include "Shader.h"



namespace SunEngine
{
	const String g_StrDefaultShaderPass = SE_DEFAULT_SHADER_PASS;
	const String g_StrShadowShaderPass = SE_SHADOW_SHADER_PASS;

	Shader::Shader() : GraphicsObject(GraphicsObject::SHADER)
	{
		_activeShaderPass = g_StrDefaultShaderPass;
	}


	Shader::~Shader()
	{
		Destroy();
	}

	bool Shader::Create(const char *configFileName)
	{
		if (!Destroy())
			return false;

		if (!_config.Load(configFileName))
		{
			_errStr = StrFormat("Failed to load shader config file %s", configFileName);
			return false;
		}

		ConfigSection* pElementSection = _config["input_elements"];
		ConfigSection::Iterator elemIt = pElementSection->GetIterator();
		while (elemIt != pElementSection->End())
		{
			APIVertexElement elem;
			elem.name = (*elemIt).first;

			OrderedStrMap<String> elemBlock;
			if (pElementSection->GetBlock(elem.name.data(), elemBlock))
			{
				elem.semantic = elemBlock["semantic"];
				elem.format = (APIVertexInputFormat)StrToInt(elemBlock["format"]);
				elem.size = StrToInt(elemBlock["size"]);
				elem.offset = StrToInt(elemBlock["offset"]);
			}

			_vertexElements.push_back(elem);
			elemIt++;
		}

		StrMap<IShaderResource> resourceLookup;
		ConfigSection* pResSection = _config["shader_resources"];
		ConfigSection::Iterator resIt = pResSection->GetIterator();
		while (resIt != pResSection->End())
		{
			String name = (*resIt).first;
			ShaderResourceType resType = SRT_UNSUPPORTED;
			uint binding = 0;
			uint stages = 0;
			String dataStr = "";
			uint flags = 0;

			OrderedStrMap<String> resBlock;
			if (pResSection->GetBlock(name.data(), resBlock))
			{
				binding = StrToInt(resBlock["binding"]);
				resType = (ShaderResourceType)StrToInt(resBlock["type"]);
				stages = StrToInt(resBlock["stages"]);
				dataStr = resBlock["dataString"];
				flags = StrToInt(resBlock["flags"]);
			}

			IShaderResource res = {};
			res.name = name;
			res.binding = binding;
			res.dataStr = dataStr;
			res.type = resType;
			res.stages = stages;
			res.size = SizeFromDataStr(dataStr);
			res.flags = flags;

			switch (resType)
			{
			case SRT_CONST_BUFFER:
				_constBufferMap[binding] = res;
				break;
			case SRT_TEXTURE:
				_textureMap[binding] = res;
				break;
			case SRT_SAMPLER:
				_samplerMap[binding] = res;
				break;
			case SRT_UNSUPPORTED:
				break;
			}

			resourceLookup[name] = res;
			resIt++;
		}

		ConfigSection* pShaderSection = 0;

		if (GetGraphicsAPI() == SE_GFX_VULKAN)
		{
			pShaderSection = _config["vk_shaders"];
		}
		else
		{
			pShaderSection = _config["dx_shaders"];
		}

		String configDir = GetDirectory(configFileName) + "/";

		StrMap<String> vertexShaders;
		StrMap<String> pixelShaders;
		StrMap<String> geometryShaders;

		ParseShaderFilesConfig("vs", pShaderSection, configDir, vertexShaders);
		ParseShaderFilesConfig("ps", pShaderSection, configDir, pixelShaders);
		ParseShaderFilesConfig("gs", pShaderSection, configDir, geometryShaders);

		ConfigSection* pPassSection = _config.GetSection("shader_passes");
		for (ConfigSection::Iterator iter = pPassSection->GetIterator(); iter != pPassSection->End(); ++iter)
		{
			String passName = (*iter).first;
			ShaderPassData& data = _iShaders[passName];

			OrderedStrMap<String> block;
			pPassSection->GetBlock(passName.data(), block);

			Vector<String> resNames;
			StrSplit(block["resources"], resNames, ';');

			IShaderCreateInfo info;
			info.vertexElements = _vertexElements;

			StrMap<String>::iterator vertPass = vertexShaders.find(passName);
			info.vertexShaderFilename = vertPass != vertexShaders.end() ? (*vertPass).second : vertexShaders[g_StrDefaultShaderPass];

			StrMap<String>::iterator pixlPass = pixelShaders.find(passName);
			info.pixelShaderFilename = pixlPass != pixelShaders.end() ? (*pixlPass).second : pixelShaders[g_StrDefaultShaderPass];

			StrMap<String>::iterator geomPass = geometryShaders.find(passName);
			info.geometryShaderFilename = geomPass != geometryShaders.end() ? (*geomPass).second : geometryShaders[g_StrDefaultShaderPass];

			for (uint i = 0; i < resNames.size(); i++)
			{
				IShaderResource res = resourceLookup[resNames[i]];
				if (res.type == SRT_TEXTURE)
				{
					data.UsedTextures.push_back(res);
					info.textures.push_back(res);
				}
				else if (res.type == SRT_SAMPLER)
				{
					data.UsedSamplers.push_back(res);
					info.samplers.push_back(res);
				}
				else if (res.type == SRT_CONST_BUFFER)
				{
					data.UsedBuffers.push_back(res);
					info.constBuffers.push_back(res);
				}
			}

			IShader* pShader = AllocateGraphics<IShader>();
			if (!pShader->Create(info))
			{
				_errStr = StrFormat("Failed to create API shader %s", configFileName);
				return false;
			}

			for (uint i = 0; i < data.UsedBuffers.size(); i++)
			{
				UniformBuffer::CreateInfo buffInfo = {};
				buffInfo.resource = data.UsedBuffers[i];
				buffInfo.pShader = pShader;

				UniformBuffer* pBuffer = new UniformBuffer();
				if (!pBuffer->Create(buffInfo))
				{
					delete pBuffer;
					return false;
				}
				
				data.ConstBuffers.insert({ buffInfo.resource.binding, pBuffer });
			}

			data.pShader = pShader;
		}

		//Give some default values to known uniform buffers...
		CameraBufferData defaultCamBuffer;
		UpdateCameraBuffer(defaultCamBuffer);

		ObjectBufferData defaultObjBuffer;
		UpdateObjectBuffer(defaultObjBuffer);

		SunlightBufferData defaultSunlightBuffer;
		defaultSunlightBuffer.Color.Set(1, 1, 1, 1);
		defaultSunlightBuffer.Direction.Set(0, 1, 0, 0);
		UpdateSunlightBuffer(defaultSunlightBuffer);

		TextureTransformBufferData defaultTexTransformBuffer;
		for (uint i = 0; i < MAX_SHADER_TEXTURE_TRANSFORMS; i++)
			defaultTexTransformBuffer.Transforms[i].Set(1, 1, 0, 0);
		UpdateTextureTransformBuffer(defaultTexTransformBuffer);

		FogBufferData defaultFog;
		defaultFog.FogColor.Set(0.5f, 0.5f, 0.5f, 1.0f);
		defaultFog.FogControls.Set(0.01f, 0.0f, 0.0f, 0.0f);
		UpdateFogBuffer(defaultFog);

		SkinnedBonesBufferData defaultBones;
		UpdateSkinnedBoneBuffer(defaultBones);

		EnvBufferData defaultEnv;
		defaultEnv.TimeData.Set(0, 0, 0, 0);
		defaultEnv.WindDir.Set(0, 0, 0, 0);
		UpdateEnvBuffer(defaultEnv);

		ShadowBufferData defaultShadow;
		UpdateShadowBuffer(defaultShadow);

		SampleSceneBufferData defaultSampleScene;
		defaultSampleScene.TexCoordTransform.Set(1, 1, 0, 0);
		defaultSampleScene.TexCoordRanges.Set(0, 0, 1, 1);
		UpdateSampleSceneBuffer(defaultSampleScene);

		RegisterShaderBindings(&_inputTextureBindings, 0, STL_COUNT - 1);

		if (ContainsTextureBinding(STL_SCENE))
		{
			_inputTextureBindings.SetTexture(SE_SCENE_TEXTURE_NAME, GraphicsContext::GetDefaultTexture(GraphicsContext::DT_WHITE));
			_inputTextureBindings.SetSampler(SE_SCENE_SAMPLER_NAME, GraphicsContext::GetDefaultSampler(GraphicsContext::DS_LINEAR_CLAMP));
		}

		if (ContainsTextureBinding(STL_DEPTH))
		{
			_inputTextureBindings.SetTexture(SE_DEPTH_TEXTURE_NAME, GraphicsContext::GetDefaultTexture(GraphicsContext::DT_WHITE));
			_inputTextureBindings.SetSampler(SE_DEPTH_SAMPLER_NAME, GraphicsContext::GetDefaultSampler(GraphicsContext::DS_LINEAR_CLAMP));
		}

		if (ContainsTextureBinding(STL_SHADOW))
		{
			_inputTextureBindings.SetTexture(SE_SHADOW_TEXTURE_NAME, GraphicsContext::GetDefaultTexture(GraphicsContext::DT_WHITE));
			_inputTextureBindings.SetSampler(SE_SHADOW_SAMPLER_NAME, GraphicsContext::GetDefaultSampler(GraphicsContext::DS_LINEAR_CLAMP));
		}

		_activeShaderPass = SE_DEFAULT_SHADER_PASS;

		return true;
	}

	IObject * Shader::GetAPIHandle() const
	{
		return _iShaders.size() ? (*_iShaders.find(_activeShaderPass)).second.pShader : 0;
	}

	void Shader::UpdateUniformBuffer(const void * pData, uint binding)
	{
		Map<uint, UniformBuffer*>& map = _iShaders.at(_activeShaderPass).ConstBuffers;
		Map<uint, UniformBuffer*>::iterator iter = map.find(binding);
		if (iter != map.end())
		{
			(*iter).second->Update(pData);
		}
	}

	void Shader::ParseShaderFilesConfig(const char* key, ConfigSection* pSection, const String& directoryToAppend, StrMap<String>& shaderPassMap)
	{
		OrderedStrMap<String> block;
		if (pSection->GetBlock(key, block))
		{
			OrderedStrMap<String>::iterator iter = block.begin();
			while (iter != block.end())
			{
				shaderPassMap[(*iter).first] = directoryToAppend + (*iter).second;
				++iter;
			}
		}
	}

	bool Shader::GetMaterialBufferDesc(IShaderResource* desc, Vector<ShaderBufferProperty>* pBufferProp) const
	{
		if (!desc)
			return false;

		Map<uint, IShaderResource>::const_iterator iter = _constBufferMap.find(SBL_MATERIAL);
		if (iter != _constBufferMap.end())
		{
			*desc = (*iter).second;

			if (pBufferProp)
			{
				pBufferProp->clear();
				Vector<String> bufferParts;
				StrSplit(desc->dataStr, bufferParts, ';');
				int offset = 0;

				for (uint i = 0; i < bufferParts.size(); i++)
				{
					ShaderBufferProperty prop;

					String propStr = bufferParts[i];
					Vector<String> propParts;
					StrSplit(propStr, propParts, ' ');

					String propTypeStr = propParts[0];

					uint numElements = 1;
					usize arrStart = propParts[1].find('[');
					usize arrEnd = propParts[1].find(']');
					if (arrStart != String::npos && arrEnd != String::npos)
					{
						String arrSizeStr = propParts[1].substr(arrStart+1, arrEnd - arrStart - 1);
						bool isNumeric = true;
						for (uint j = 0; j < arrSizeStr.size() && isNumeric; j++)
						{
							if (!(arrSizeStr[j] >= '0' && arrSizeStr[j] <= '9'))
								isNumeric = false;
						}

						if (isNumeric)
							numElements = StrToInt(arrSizeStr);

						prop.Name = propParts[1].substr(0, arrStart);
					}
					else
					{
						prop.Name = propParts[1];
					}

					prop.Offset = offset;

					ShaderDataType propType = SDT_UNDEFINED;
					int propSize = 0;

					if (propTypeStr == "float")
					{
						propType = SDT_FLOAT;
						propSize = sizeof(float);
					}
					else if (propTypeStr == "float2")
					{
						propType = SDT_FLOAT2;
						propSize = sizeof(float) * 2;
					}
					else if (propTypeStr == "float3")
					{
						propType = SDT_FLOAT3;
						propSize = sizeof(float) * 3;
					}
					else if (propTypeStr == "float4")
					{
						propType = SDT_FLOAT4;
						propSize = sizeof(float) * 4;
					}
					else if (propTypeStr == "float2x2")
					{
						propType = SDT_MAT2;
						propSize = sizeof(float) * 2 * 2;
					}
					else if (propTypeStr == "float3x3")
					{
						propType = SDT_MAT3;
						propSize = sizeof(float) * 3 * 3;
					}
					else if (propTypeStr == "float4x4")
					{
						propType = SDT_MAT4;
						propSize = sizeof(float) * 4 * 4;
					}

					propSize *= numElements;
					offset += propSize;

					prop.Type = propType;
					prop.Size = propSize;
					prop.NumElements = numElements;

					pBufferProp->push_back(prop);
				}
			}

			return true;
		}
		else
		{
			return false;
		}
	}
 
	void Shader::GetTextureDescs(Vector<IShaderResource>& desc) const
	{
		Map<uint, IShaderResource>::const_iterator iter = _textureMap.begin();
		while (iter != _textureMap.end())
		{
			desc.push_back((*iter).second);
			iter++;
		}
	}

	void Shader::GetSamplerDescs(Vector<IShaderResource>& desc) const
	{
		Map<uint, IShaderResource>::const_iterator iter = _samplerMap.begin();
		while (iter != _samplerMap.end())
		{
			desc.push_back((*iter).second);
			iter++;
		}
	}

	void Shader::UpdateCameraBuffer(const CameraBufferData & data)
	{
		_cameraBuffer = data;
		this->UpdateUniformBuffer(&data, SBL_CAMERA_BUFFER);
	}

	void Shader::UpdateObjectBuffer(const ObjectBufferData & data)
	{
		_objectBuffer = data;
		this->UpdateUniformBuffer(&data, SBL_OBJECT_BUFFER);
	}

	void Shader::UpdateMaterialBuffer(const void * pData)
	{
		this->UpdateUniformBuffer(pData, SBL_MATERIAL);
	}

	void Shader::UpdateSunlightBuffer(const SunlightBufferData& data)
	{
		_sunlightBuffer = data;
		this->UpdateUniformBuffer(&data, SBL_SUN_LIGHT);
	}

	void Shader::UpdateTextureTransformBuffer(const TextureTransformBufferData & data)
	{
		this->UpdateUniformBuffer(&data, SBL_TEXTURE_TRANSFORM);
	}

	void Shader::UpdateFogBuffer(const FogBufferData& data)
	{
		_fogBuffer = data;
		this->UpdateUniformBuffer(&data, SBL_FOG);
	}

	void Shader::UpdateSkinnedBoneBuffer(const SkinnedBonesBufferData& data)
	{
		this->UpdateUniformBuffer(&data, SBL_SKINNED_BONES);
	}

	void Shader::UpdateEnvBuffer(const EnvBufferData& data)
	{
		_envBuffer = data;
		this->UpdateUniformBuffer(&data, SBL_ENV);
	}

	void Shader::UpdateShadowBuffer(const ShadowBufferData& data)
	{
		this->UpdateUniformBuffer(&data, SBL_SHADOW_BUFFER);
	}

	void Shader::UpdateSampleSceneBuffer(const SampleSceneBufferData& data)
	{
		this->UpdateUniformBuffer(&data, SBL_SAMPLE_SCENE);
	}

	uint Shader::SizeFromDataStr(const String & str) const
	{
		Vector<String> parts;
		StrSplit(str, parts, ';');

		uint size = 0;

		for (uint i = 0; i < parts.size(); i++)
		{
			Vector<String> subParts;
			StrSplit(parts[i], subParts, ' ');
			
			String strType = subParts[0];
			
			uint typeSize = 0;

			if (strType == "int")
				typeSize = sizeof(int);
			if (strType == "float")
				typeSize = sizeof(float);
			if(strType == "uint")
				typeSize = sizeof(uint);

			if (strType == "int2")
				typeSize = sizeof(int) * 2;
			if (strType == "float2")
				typeSize = sizeof(float) * 2;
			if (strType == "uint2")
				typeSize = sizeof(uint) * 2;

			if (strType == "int3")
				typeSize = sizeof(int) * 3;
			if (strType == "float3")
				typeSize = sizeof(float) * 3;
			if (strType == "uint3")
				typeSize = sizeof(uint) * 3;


			if (strType == "int4")
				typeSize = sizeof(int) * 4;
			if (strType == "float4")
				typeSize = sizeof(float) * 4;
			if (strType == "uint4")
				typeSize = sizeof(uint) * 4;


			if (strType == "float2x2")
				typeSize = sizeof(float) * 2 * 2;
			if (strType == "float3x3")
				typeSize = sizeof(float) * 3 * 3;
			if (strType == "float4x4")
				typeSize = sizeof(float) * 4 * 4;

			if (subParts.size() > 1)
			{
				usize arrStart = subParts[1].find('[');
				usize arrEnd = subParts[1].find(']');
				if (arrStart != String::npos && arrEnd != String::npos)
				{

					String arrSizeStr = subParts[1].substr(arrStart+1, arrEnd - arrStart - 1);
					bool isNumeric = true;
					for (uint j = 0; j < arrSizeStr.size() && isNumeric; j++)
					{
						if (!(arrSizeStr[j] >= '0' && arrSizeStr[j] <= '9'))
							isNumeric = false;
					}

					if (isNumeric)
						typeSize *= StrToInt(arrSizeStr);
				}
			}

			size += typeSize;

		}

		return size;
	}

	bool Shader::RegisterShaderBindings(ShaderBindings* pBindings, uint minBinding, uint maxBindings)
	{
		if (!pBindings->Destroy())
			return false;

		pBindings->_shader = this;

		StrMap<ShaderPassData>::iterator iter = _iShaders.begin();
		while (iter != _iShaders.end())
		{
			ShaderPassData& data = (*iter).second;
			IShaderBindings* passBindings = AllocateGraphics<IShaderBindings>();

			Vector<IShaderResource> exposedTextures;
			Vector<IShaderResource> exposedSamplers;

			for (uint i = 0; i < data.UsedTextures.size(); i++)
			{
				if (data.UsedTextures[i].binding >= minBinding && data.UsedTextures[i].binding <= maxBindings)
					exposedTextures.push_back(data.UsedTextures[i]);
			}

			for (uint i = 0; i < data.UsedSamplers.size(); i++)
			{
				if (data.UsedSamplers[i].binding >= minBinding && data.UsedSamplers[i].binding <= maxBindings)
					exposedSamplers.push_back(data.UsedSamplers[i]);
			}

			if (!passBindings->Create((*iter).second.pShader, exposedTextures, exposedSamplers)) //TODO: status return here?
				return false;
			pBindings->_iBindings[(*iter).first] = passBindings;
			++iter;
		}

		for (Map<uint, IShaderResource>::iterator resIter = _textureMap.begin(); resIter != _textureMap.end(); ++resIter)
		{
			if ((*resIter).first >= minBinding && (*resIter).first <= maxBindings)
				pBindings->_resourceMap[(*resIter).second.name].Binding = (*resIter).first;
		}

		for (Map<uint, IShaderResource>::iterator resIter = _samplerMap.begin(); resIter != _samplerMap.end(); ++resIter)
		{
			if ((*resIter).first >= minBinding && (*resIter).first <= maxBindings)
				pBindings->_resourceMap[(*resIter).second.name].Binding = (*resIter).first;
		}

		return true;
	}

	String Shader::GetCompilerCommandLine()
	{
		ConfigSection* pSection = _config.GetSection("source");
		if (!pSection)
			return "";

		return pSection->GetString("cmdLine");
	}

	const String &Shader::GetConfigFilename() const
	{
		return _config.GetFilename();
	}

	bool Shader::Destroy()
	{
		StrMap<ShaderPassData>::iterator iter = _iShaders.begin();
		while (iter != _iShaders.end())
		{
			IShader* pShader = (*iter).second.pShader;

			if (!pShader->Destroy())
				return false;

			delete pShader;

			Map<uint, UniformBuffer*>::iterator buffIter = (*iter).second.ConstBuffers.begin();
			while (buffIter != (*iter).second.ConstBuffers.end())
			{
				(*buffIter).second->Destroy();
				delete (*buffIter).second;
				++buffIter;
			}

			++iter;
		}



		_constBufferMap.clear();
		_samplerMap.clear();
		_textureMap.clear();
		_vertexElements.clear();
		_iShaders.clear();

		return true;

	}

	bool Shader::RegisterShaderBindings(ShaderBindings * pBindings)
	{	
		return RegisterShaderBindings(pBindings, STL_COUNT, UINT32_MAX);
	}

	bool Shader::ContainsTextureBinding(uint binding) const
	{
		return _textureMap.find(binding) != _textureMap.end();
	}

	bool Shader::ContainsSamplerBinding(uint binding) const
	{
		return _samplerMap.find(binding) != _samplerMap.end();
	}

	void Shader::SetActiveShaderPass(const String& passName)
	{
		_activeShaderPass = passName;
	}

	void Shader::SetDefaultShaderPass()
	{
		SetActiveShaderPass(g_StrDefaultShaderPass);
	}

	void Shader::SetShadowShaderPass()
	{
		SetActiveShaderPass(g_StrShadowShaderPass);
	}

	bool Shader::SupportsShaderPass(const String & passName)
	{
		return _iShaders.find(passName) != _iShaders.end();
	}

	bool Shader::OnBind(CommandBuffer* cmdBuffer)
	{
		Map<uint, UniformBuffer*>& map = _iShaders.at(_activeShaderPass).ConstBuffers;
		for (Map<uint, UniformBuffer*>::iterator iter = map.begin(); iter != map.end(); ++iter)
		{
			(*iter).second->Bind(cmdBuffer);
		}

		_inputTextureBindings.Bind(cmdBuffer);

		return true;
	}

	bool Shader::OnUnbind(CommandBuffer* cmdBuffer)
	{
		Map<uint, UniformBuffer*>& map = _iShaders.at(_activeShaderPass).ConstBuffers;
		for (Map<uint, UniformBuffer*>::iterator iter = map.begin(); iter != map.end(); ++iter)
		{
			(*iter).second->Unbind(cmdBuffer);
		}

		_inputTextureBindings.Unbind(cmdBuffer);

		return true;
	}

	void Shader::UpdateInputTexture(const String& name, BaseTexture* pTexture)
	{
		_inputTextureBindings.SetTexture(name, pTexture);
	}

	void Shader::UpdateInputSampler(const String& name, Sampler* pSampler)
	{
		_inputTextureBindings.SetSampler(name, pSampler);
	}

	const String & Shader::GetActiveShaderPass() const
	{
		return _activeShaderPass;
	}

	ShaderBindings::ShaderBindings() : GraphicsObject(GraphicsObject::SHADER_BINDINGS)
	{
	}

	IObject * ShaderBindings::GetAPIHandle() const
	{
		return _iBindings.size() ? (*_iBindings.find(_shader->GetActiveShaderPass())).second : 0;
	}

	bool ShaderBindings::OnBind(CommandBuffer* cmdBuffer)
	{
		return GraphicsObject::OnBind(cmdBuffer);
	}

	bool ShaderBindings::OnUnbind(CommandBuffer* cmdBuffer)
	{
		return GraphicsObject::OnUnbind(cmdBuffer);
	}

	bool ShaderBindings::SetTexture(const String& name, BaseTexture* pTexture)
	{
		StrMap<ResourceInfo>::iterator foundIter = _resourceMap.find(name);
		if (foundIter != _resourceMap.end())
		{
			if ((*foundIter).second.Resource == pTexture && (*foundIter).second.CurrentHandle == pTexture->GetAPIHandle())
				return true;

			uint binding = (*foundIter).second.Binding;
			StrMap<IShaderBindings*>::iterator iter = _iBindings.begin();
			while (iter != _iBindings.end())
			{
				(*iter).second->SetTexture((ITexture*)pTexture->GetAPIHandle(), binding);
				++iter;
			}
			(*foundIter).second.Set(pTexture);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool ShaderBindings::SetSampler(const String& name, Sampler * pSampler)
	{
		StrMap<ResourceInfo>::iterator foundIter = _resourceMap.find(name);
		if (foundIter != _resourceMap.end())
		{
			if ((*foundIter).second.Resource == pSampler && (*foundIter).second.CurrentHandle == pSampler->GetAPIHandle())
				return true;

			uint binding = (*foundIter).second.Binding;
			StrMap<IShaderBindings*>::iterator iter = _iBindings.begin();
			while (iter != _iBindings.end())
			{
				(*iter).second->SetSampler((ISampler*)pSampler->GetAPIHandle(), binding);
				++iter;
			}
			(*foundIter).second.Set(pSampler);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool ShaderBindings::SetTextureCube(const String& name, BaseTextureCube* pTextureCube)
	{
		StrMap<ResourceInfo>::iterator foundIter = _resourceMap.find(name);
		if (foundIter != _resourceMap.end())
		{
			if ((*foundIter).second.Resource == pTextureCube && (*foundIter).second.CurrentHandle == pTextureCube->GetAPIHandle())
				return true;

			uint binding = (*foundIter).second.Binding;
			StrMap<IShaderBindings*>::iterator iter = _iBindings.begin();
			while (iter != _iBindings.end())
			{
				(*iter).second->SetTextureCube((ITextureCube*)pTextureCube->GetAPIHandle(), binding);
				++iter;
			}
			(*foundIter).second.Set(pTextureCube);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool ShaderBindings::SetTextureArray(const String& name, BaseTextureArray* pTextureArray)
	{
		StrMap<ResourceInfo>::iterator foundIter = _resourceMap.find(name);
		if (foundIter != _resourceMap.end())
		{
			if ((*foundIter).second.Resource == pTextureArray && (*foundIter).second.CurrentHandle == pTextureArray->GetAPIHandle())
				return true;

			uint binding = (*foundIter).second.Binding;
			StrMap<IShaderBindings*>::iterator iter = _iBindings.begin();
			while (iter != _iBindings.end())
			{
				(*iter).second->SetTextureArray((ITextureArray*)pTextureArray->GetAPIHandle(), binding);
				++iter;
			}

			(*foundIter).second.Set(pTextureArray);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool ShaderBindings::Destroy()
	{
		StrMap<IShaderBindings*>::iterator iter = _iBindings.begin();
		while (iter != _iBindings.end())
		{
			if (!(*iter).second->Destroy())
				return false;
			delete (*iter).second;
			++iter;
		}

		_iBindings.clear();
		_shader = 0;
		_resourceMap.clear();
		return true;
	}

	bool ShaderBindings::ContainsResource(const String& name) const
	{
		return _resourceMap.find(name) != _resourceMap.end();
	}

	//bool ShaderBindings::UpdateIfChanged()
	//{
	//	uint numChanged = 0;

	//	//update anything that might have been recreated...likely will be issues if vulkan stuff is ever gotten to work again
	//	ResourceMap::iterator iter = _resourceMap.begin();
	//	while (iter != _resourceMap.end())
	//	{
	//		uint type = (*iter).first;

	//		StrMap<ResourceInfo>::iterator subIter = (*iter).second.begin();
	//		while (subIter != (*iter).second.end())
	//		{
	//			ResourceInfo& current = (*subIter).second;
	//			if (current.CurrentHandle != current.Resource->GetAPIHandle())
	//			{
	//				if (type == GraphicsObject::TEXTURE)
	//				{
	//					SetTexture((*subIter).first, static_cast<BaseTexture*>(current.Resource));
	//				}
	//				else if (type == GraphicsObject::SAMPLER)
	//				{
	//					SetSampler((*subIter).first, static_cast<Sampler*>(current.Resource));
	//				}
	//				else if (type == GraphicsObject::TEXTURE_CUBE)
	//				{
	//					SetTextureCube((*subIter).first, static_cast<BaseTextureCube*>(current.Resource));
	//				}
	//				else if (type == GraphicsObject::TEXTURE_ARRAY)
	//				{
	//					SetTextureArray((*subIter).first, static_cast<BaseTextureArray*>(current.Resource));
	//				}

	//				numChanged++;
	//			}
	//			++subIter;
	//		}
	//		++iter;
	//	}

	//	return numChanged > 0;
	//}

	ShaderBindings::ResourceInfo::ResourceInfo()
	{
		Resource = 0;
		CurrentHandle = 0;
	}

	void ShaderBindings::ResourceInfo::Set(GraphicsObject* pObject)
	{
		Resource = pObject;
		CurrentHandle = pObject->GetAPIHandle();
	}

	ShaderMat4::ShaderMat4()
	{
		data[0] = 1.0f;
		data[1] = 0.0f;
		data[2] = 0.0f;
		data[3] = 0.0f;

		data[4] = 0.0f;
		data[5] = 1.0f;
		data[6] = 0.0f;
		data[7] = 0.0f;

		data[8] = 0.0f;
		data[9] = 0.0f;
		data[10] = 1.0f;
		data[11] = 0.0f;

		data[12] = 0.0f;
		data[13] = 0.0f;
		data[14] = 0.0f;
		data[15] = 1.0f;
	}

	ShaderVec4::ShaderVec4()
	{
		data[0] = 0.0f;
		data[1] = 0.0f;
		data[2] = 0.0f;
		data[3] = 0.0f;
	}

	void ShaderVec4::Set(float x, float y, float z, float w)
	{
		data[0] = x;
		data[1] = y;
		data[2] = z;
		data[3] = w;
	}

}