#include "Shader.h"
#include "Texture2D.h"
#include "Material.h"

namespace SunEngine
{
	Material::Material()
	{
		_shader = 0;
	}

	Material::~Material()
	{

	}

	void Material::SetShader(Shader* pShader)
	{
		_shader = pShader;
	}

	bool Material::SetMaterialVar(const String& name, const void* pData, const uint size)
	{
		auto found = _mtlVariables.find(name);
		if (found != _mtlVariables.end() && (*found).second.Size <= size)
		{
			_memBuffer.SetData(pData, size, (*found).second.Offset);
			if (!_mtlBuffer.Update(_memBuffer.GetData()))
				return false;

			return true;
		}
		else
		{
			return false;
		}
	}

	bool Material::SetTexture2D(const String& name, Texture2D* pTexture)
	{
		auto found = _mtlTextures2D.find(name);
		if (found != _mtlTextures2D.end())
		{
			if (!_gpuObject.SetTexture(name, pTexture->GetGPUObject()))
				return false;

			(*found).second.pTexture = pTexture;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Material::SetSampler(const String& name, Sampler* pSampler)
	{
		auto found = _mtlSamplers.find(name);
		if (found != _mtlSamplers.end())
		{
			if (!_gpuObject.SetSampler(name, pSampler))
				return false;

			return true;
		}
		else
		{
			return false;
		}
	}

	bool Material::RegisterToGPU()
	{
		if (!_shader)
			return false;

		if (!_gpuObject.Destroy())
			return false;

		ShaderBindings::CreateInfo info = {};
		info.pShader = _shader->GetGPUObject();
		info.type = SBT_MATERIAL;

		if (!_gpuObject.Create(info))
			return false;

		_mtlVariables.clear();
		_mtlTextures2D.clear();
		_mtlSamplers.clear();

		if (_shader)
		{
			Vector<IShaderBuffer> buffInfos;
			Vector<IShaderResource> resInfos;
			_shader->GetGPUObject()->GetBufferInfos(buffInfos);
			_shader->GetGPUObject()->GetResourceInfos(resInfos);

			for (uint i = 0; i < buffInfos.size(); i++)
			{
				if (buffInfos[i].name == ShaderStrings::MaterialBufferName && buffInfos[i].bindType == SBT_MATERIAL)
				{
					IShaderBuffer& buff = buffInfos[i];

					UniformBuffer::CreateInfo buffCreateInfo = {};
					buffCreateInfo.isShared = false;
					buffCreateInfo.size = buff.size;
					if (!_mtlBuffer.Create(buffCreateInfo))
						return false;

					_memBuffer.SetSize(buff.size);
					if (!_gpuObject.SetUniformBuffer(buff.name, &_mtlBuffer))
						return false;

					for (uint j = 0; j < buff.Variables.size(); j++)
					{
						_mtlVariables[buff.Variables[j].Name] = buff.Variables[j];
					}

					break;
				}
			}

			for (uint i = 0; i < resInfos.size(); i++)
			{
				if (resInfos[i].bindType == SBT_MATERIAL)
				{
					IShaderResource& res = resInfos[i];
					if (res.type == SRT_TEXTURE)
					{
						if (res.dimension == SRD_TEXTURE_2D)
						{
							MaterialTextureData data = {};
							data.Res = res;
							_mtlTextures2D[res.name] = data;
						}
					}
					else if (res.type == SRT_SAMPLER)
					{
						MaterialSamplerData data = {};
						data.Res = res;
						_mtlSamplers[res.name] = data;
					}
				}
			}

			_shader->SetDefaults(this);
		}

		return true;
	}

	void Material::BuildPipelineSettings(PipelineSettings& settings) const
	{
		if (_shader)
		{

		}
	}
}