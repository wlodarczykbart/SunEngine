#include <assert.h>

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
#include "BaseShader.h"



namespace SunEngine
{
	String ShaderStrings::CameraBufferName = "CameraBuffer";
	String ShaderStrings::ObjectBufferName = "ObjectBuffer";
	String ShaderStrings::EnvBufferName = "EnvBuffer";
	String ShaderStrings::FogBufferName = "FogBuffer";
	String ShaderStrings::SkinnedBoneBufferName = "SkinnedBoneBuffer";
	String ShaderStrings::SunlightBufferName = "SunlightBuffer";
	String ShaderStrings::PointlightBufferName = "PointlightBuffer";
	String ShaderStrings::SpotlightBufferName = "SpotlightBuffer";
	String ShaderStrings::TextureTransformBufferName = "TextureTransformBuffer";
	String ShaderStrings::MaterialBufferName = "MaterialBuffer";
	String ShaderStrings::ShadowBufferName = "ShadowBuffer";

	String ShaderStrings::SceneTextureName = "SceneTexture";
	String ShaderStrings::SceneSamplerName = "SceneSampler";
	String ShaderStrings::DepthTextureName = "DepthTexture";
	String ShaderStrings::DepthSamplerName = "DepthSampler";
	String ShaderStrings::ShadowTextureName = "ShadowTexture";
	String ShaderStrings::ShadowSamplerName = "ShadowSampler";

	BaseShader::BaseShader() : GraphicsObject(GraphicsObject::SHADER)
	{
		_iShader = 0;
	}


	BaseShader::~BaseShader()
	{
		Destroy();
	}

	bool BaseShader::Create(const CreateInfo& info)
	{
		if (!Destroy())
			return false;

		_buffers = info.buffers;
		_resources = info.resources;

		IShaderCreateInfo apiInfo = {};
		apiInfo.buffers = info.buffers;
		apiInfo.resources = info.resources;
		apiInfo.vertexElements = info.vertexElements;

		for (uint i = 0; i < SE_ARR_SIZE(info.vertexBinaries); i++)
		{
			apiInfo.vertexBinaries[i] = info.vertexBinaries[i];
			apiInfo.pixelBinaries[i] = info.pixelBinaries[i];
			apiInfo.geometryBinaries[i] = info.geometryBinaries[i];
		}

		_iShader = AllocateGraphics<IShader>();
		if (!_iShader->Create(apiInfo))
			return false;

		return true;
	}

	bool BaseShader::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_resources.clear();
		_buffers.clear();
		_iShader = 0;
		return true;
	}

	IObject * BaseShader::GetAPIHandle() const
	{
		return _iShader;
	}

	void BaseShader::GetBufferInfos(Vector<IShaderBuffer>& infos) const
	{
		for (auto iter = _buffers.begin(); iter != _buffers.end(); ++iter)
		{
			infos.push_back((*iter).second);
		}
	}

	void BaseShader::GetResourceInfos(Vector<IShaderResource>& infos) const
	{
		for (auto iter = _resources.begin(); iter != _resources.end(); ++iter)
		{
			infos.push_back((*iter).second);
		}
	}

	bool BaseShader::ContainsBuffer(const String& name) const
	{
		return _buffers.find(name) != _buffers.end();
	}

	bool BaseShader::ContainsResource(const String& name) const
	{
		return _resources.find(name) != _resources.end();
	}

	ShaderBindings::ShaderBindings() : GraphicsObject(GraphicsObject::SHADER_BINDINGS)
	{
		_shader = 0;
		_iBindings = 0;
	}

	IObject * ShaderBindings::GetAPIHandle() const
	{
		return _iBindings;
	}

	bool ShaderBindings::SetUniformBuffer(const String& name, UniformBuffer* pBuffer)
	{
		StrMap<ResourceInfo>::iterator foundIter = _resourceMap.find(name);
		if (foundIter != _resourceMap.end())
		{
			if ((*foundIter).second.Resource == pBuffer && (*foundIter).second.CurrentHandle == pBuffer->GetAPIHandle())
				return true;

			const String& binding = (*foundIter).second.Name;
			_iBindings->SetUniformBuffer((IUniformBuffer*)pBuffer->GetAPIHandle(), binding);
			(*foundIter).second.Set(pBuffer);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool ShaderBindings::SetTexture(const String& name, BaseTexture* pTexture)
	{
		StrMap<ResourceInfo>::iterator foundIter = _resourceMap.find(name);
		if (foundIter != _resourceMap.end())
		{
			//if ((*foundIter).second.Resource == pTexture && (*foundIter).second.CurrentHandle == pTexture->GetAPIHandle())
			//	return true;

			const String& binding = (*foundIter).second.Name;
			_iBindings->SetTexture((ITexture*)pTexture->GetAPIHandle(), binding);
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
			//if ((*foundIter).second.Resource == pSampler && (*foundIter).second.CurrentHandle == pSampler->GetAPIHandle())
			//	return true;

			const String& binding = (*foundIter).second.Name;
			_iBindings->SetSampler((ISampler*)pSampler->GetAPIHandle(), binding);
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
			//if ((*foundIter).second.Resource == pTextureCube && (*foundIter).second.CurrentHandle == pTextureCube->GetAPIHandle())
			//	return true;

			const String& binding = (*foundIter).second.Name;
			_iBindings->SetTextureCube((ITextureCube*)pTextureCube->GetAPIHandle(), binding);
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
			//if ((*foundIter).second.Resource == pTextureArray && (*foundIter).second.CurrentHandle == pTextureArray->GetAPIHandle())
			//	return true;

			const String& binding = (*foundIter).second.Name;
			_iBindings->SetTextureArray((ITextureArray*)pTextureArray->GetAPIHandle(), binding);
			(*foundIter).second.Set(pTextureArray);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool ShaderBindings::Create(const CreateInfo& info)
	{
		if (!Destroy())
			return false;

		if (!info.pShader)
			return false;

		_shader = (BaseShader*)info.pShader;

		IShaderBindingCreateInfo bindingInfo = {};
		bindingInfo.pShader = static_cast<IShader*>(_shader->GetAPIHandle());
		bindingInfo.type = info.type;

		Vector<IShaderBuffer> buffers;
		_shader->GetBufferInfos(buffers);

		Vector<IShaderResource> resources;
		_shader->GetResourceInfos(resources);

		for (uint i = 0; i < buffers.size(); i++)
		{
			if (buffers[i].bindType == info.type)
			{
				const IShaderBuffer& buff = buffers[i];
				bindingInfo.bufferBindings.push_back(buff);

				ResourceInfo resInfo = {};
				resInfo.Name = buff.name;
				assert(resInfo.Name.length());
				_resourceMap[buff.name] = resInfo;
			}
		}

		for (uint i = 0; i < resources.size(); i++)
		{
			if (resources[i].bindType == info.type)
			{
				const IShaderResource& res = resources[i];
				bindingInfo.resourceBindings.push_back(res);

				ResourceInfo resInfo = {};
				resInfo.Name = res.name;
				assert(resInfo.Name.length());
				_resourceMap[res.name] = resInfo;
			}
		}

		_iBindings = AllocateGraphics<IShaderBindings>();

		//only create the api bindings if any buffers our resources are using the binding type
		if (bindingInfo.bufferBindings.size() || bindingInfo.resourceBindings.size())
		{
			if (!_iBindings->Create(bindingInfo))
				return false;
		}

		return true;
	}

	bool ShaderBindings::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_iBindings = 0;
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

	void ShaderMat4::Set(const void* pData)
	{
		memcpy(data, pData, sizeof(data));
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

	void ShaderVec4::Set(const void* pData)
	{
		memcpy(data, pData, sizeof(data));
	}

}