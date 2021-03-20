#include <assert.h>

#include "ConfigFile.h"
#include "StringUtil.h"
#include "FileReader.h"

#include "D3D11CommandBuffer.h"
#include "D3D11RenderTarget.h"
#include "D3D11Sampler.h"
#include "D3D11UniformBuffer.h"
#include "D3D11Texture.h"
#include "D3D11TextureCube.h"
#include "D3D11TextureArray.h"

#include "Sampler.h"
#include "BaseTexture.h"
#include "GraphicsContext.h"

#include "MemBuffer.h"

#include "D3D11Shader.h"

namespace SunEngine
{
	D3D11Shader::D3D11Shader()
	{
		_vertexShader = 0;
		_pixelShader = 0;
		_geometryShader = 0;
		_inputLayout = 0;
	}


	D3D11Shader::~D3D11Shader()
	{
		Destroy();
	}

	bool D3D11Shader::Create(IShaderCreateInfo& info)
	{

		if (!_device->CreateVertexShader(info.vertexBinaries[SE_GFX_D3D11].GetData(), info.vertexBinaries[SE_GFX_D3D11].GetSize(), &_vertexShader)) return false;

		if (info.pixelBinaries[SE_GFX_D3D11].GetSize())
		{
			if (!_device->CreatePixelShader(info.pixelBinaries[SE_GFX_D3D11].GetData(), info.pixelBinaries[SE_GFX_D3D11].GetSize(), &_pixelShader)) return false;
		}

		if (info.geometryBinaries[SE_GFX_D3D11].GetSize())
		{
			if (!_device->CreateGeometryShader(info.geometryBinaries[SE_GFX_D3D11].GetData(), info.geometryBinaries[SE_GFX_D3D11].GetSize(), &_geometryShader)) return false;
		}

		Vector<D3D11_INPUT_ELEMENT_DESC> elements;
		for (uint i = 0; i < info.vertexElements.size(); i++)
		{
			IVertexElement &data = info.vertexElements[i];

			D3D11_INPUT_ELEMENT_DESC elem = {};
			elem.AlignedByteOffset = data.offset;
			elem.SemanticName = data.semantic;
			elem.InstanceDataStepRate = D3D11_INPUT_PER_VERTEX_DATA;

			switch (data.format)
			{
			case IVertexInputFormat::VIF_FLOAT2:
				elem.Format = DXGI_FORMAT_R32G32_FLOAT;
				break;
			case IVertexInputFormat::VIF_FLOAT3:
				elem.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case IVertexInputFormat::VIF_FLOAT4:
				elem.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			default:
				break;
			}
			elements.push_back(elem);
		}

		if (elements.size())
		{
			if (!_device->CreateInputLayout(elements.data(), (uint)elements.size(), info.vertexBinaries[SE_GFX_D3D11].GetData(), info.vertexBinaries[SE_GFX_D3D11].GetSize(), &_inputLayout)) return false;
		}

		for(auto iter = info.buffers.begin(); iter != info.buffers.end(); ++iter)
		{
			IShaderBuffer& buff = (*iter).second;

			Vector<BindConstantBufferFunc>& funcs = _constBufferFuncMap[buff.binding[SE_GFX_D3D11]];
			if (buff.stages & SS_VERTEX)
				funcs.push_back(&D3D11Shader::BindConstantBufferVS);
			if (buff.stages & SS_PIXEL)
				funcs.push_back(&D3D11Shader::BindConstantBufferPS);
			if (buff.stages & SS_GEOMETRY)
				funcs.push_back(&D3D11Shader::BindConstantBufferGS);
		}

		for (auto iter = info.resources.begin(); iter != info.resources.end(); ++iter)
		{
			if ((*iter).second.type == SRT_TEXTURE)
			{
				IShaderResource& tex = (*iter).second;

				Vector<BindShaderResourceFunc>& funcs = _textureFuncMap[tex.binding[SE_GFX_D3D11]];
				if (tex.stages & SS_VERTEX)
					funcs.push_back(&D3D11Shader::BindShaderResourceVS);
				if (tex.stages & SS_PIXEL)
					funcs.push_back(&D3D11Shader::BindShaderResourcePS);
				if (tex.stages & SS_GEOMETRY)
					funcs.push_back(&D3D11Shader::BindShaderResourceGS);
			}
		}

		for (auto iter = info.resources.begin(); iter != info.resources.end(); ++iter)
		{
			if ((*iter).second.type == SRT_SAMPLER)
			{
				IShaderResource& sampler = (*iter).second;

				Vector<BindSamplerFunc>& funcs = _samplerFuncMap[sampler.binding[SE_GFX_D3D11]];
				if (sampler.stages & SS_VERTEX)
					funcs.push_back(&D3D11Shader::BindSamplerVS);
				if (sampler.stages & SS_PIXEL)
					funcs.push_back(&D3D11Shader::BindSamplerPS);
				if (sampler.stages & SS_GEOMETRY)
					funcs.push_back(&D3D11Shader::BindSamplerPS);
			}
		}

		return true;
	}

	bool D3D11Shader::Destroy()
	{
		COM_RELEASE(_vertexShader);
		COM_RELEASE(_pixelShader);
		COM_RELEASE(_geometryShader);
		COM_RELEASE(_inputLayout);

		_constBufferFuncMap.clear();
		_samplerFuncMap.clear();
		_textureFuncMap.clear();

		return true;
	}

	void D3D11Shader::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;
		dxCmd->SetCurrentShader(this);

		if(_inputLayout) dxCmd->BindInputLayout(_inputLayout);
		if(_vertexShader) dxCmd->BindVertexShader(_vertexShader);
		if(_pixelShader) dxCmd->BindPixelShader(_pixelShader);
		if(_geometryShader) dxCmd->BindGeometryShader(_geometryShader);
	}

	void D3D11Shader::Unbind(ICommandBuffer * cmdBuffer)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;
		dxCmd->BindGeometryShader(0);
		dxCmd->BindInputLayout(0);

		(void)cmdBuffer;
		ID3D11ShaderResourceView* pNullSRV = 0;
		Map<uint, Vector<BindShaderResourceFunc> >::iterator texIter = _textureFuncMap.begin();
		while (texIter != _textureFuncMap.end())
		{
			uint binding = (*texIter).first;
			Vector<BindShaderResourceFunc> &funcs = (*texIter).second;
			for (uint i = 0; i < funcs.size(); i++)
			{
				(this->*funcs[i])(pNullSRV, binding);
			}
			texIter++;
		}

		ID3D11SamplerState* pNullSampler = 0;
		Map<uint, Vector<BindSamplerFunc> >::iterator samplerIt = _samplerFuncMap.begin();
		while (samplerIt != _samplerFuncMap.end())
		{
			uint binding = (*samplerIt).first;
			Vector<BindSamplerFunc> &funcs = (*samplerIt).second;
			for (uint i = 0; i < funcs.size(); i++)
			{
				(this->*funcs[i])(pNullSampler, binding);
			}
			samplerIt++;
		}

		ID3D11Buffer* pNullBuffer = 0;
		Map<uint, Vector<BindConstantBufferFunc>  >::iterator bufferIt = _constBufferFuncMap.begin();
		while (bufferIt != _constBufferFuncMap.end())
		{
			uint binding = (*bufferIt).first;
			Vector<BindConstantBufferFunc> &funcs = (*bufferIt).second;
			for (uint i = 0; i < funcs.size(); i++)
			{
				(this->*funcs[i])(pNullBuffer, binding, 0, 0);
			}
			bufferIt++;
		}

		dxCmd->SetCurrentShader(0);
	}

	void D3D11Shader::BindTexture(D3D11Texture * pTexture, uint binding)
	{
		BindShaderResourceView(pTexture->_srv, binding);
	}

	void D3D11Shader::BindTextureCube(D3D11TextureCube * pTextureCube, uint binding)
	{
		BindShaderResourceView(pTextureCube->_srv, binding);
	}

	void D3D11Shader::BindTextureArray(D3D11TextureArray * pTextureArray, uint binding)
	{
		BindShaderResourceView(pTextureArray->_srv, binding);
	}

	void D3D11Shader::BindSampler(D3D11Sampler * pSampler, uint binding)
	{
		Map<uint, Vector<BindSamplerFunc> >::iterator it = _samplerFuncMap.find(binding);
		if (it != _samplerFuncMap.end())
		{
			Vector<BindSamplerFunc> &funcs = (*it).second;
			for (uint i = 0; i < funcs.size(); i++)
			{
				(this->*funcs[i])(pSampler->_sampler, binding);
			}
		}
	}

	void D3D11Shader::BindBuffer(D3D11UniformBuffer* pBuffer, uint binding, uint firstConstant, uint numConstants)
	{
		Map<uint, Vector<BindConstantBufferFunc> >::iterator it = _constBufferFuncMap.find(binding);
		if (it != _constBufferFuncMap.end())
		{
			Vector<BindConstantBufferFunc>& funcs = (*it).second;
			for (uint i = 0; i < funcs.size(); i++)
			{
				(this->*funcs[i])(pBuffer->_buffer, binding, firstConstant, numConstants);
			}
		}
	}

	void D3D11Shader::BindShaderResourceView(ID3D11ShaderResourceView * pSRV, uint binding)
	{
		Map<uint, Vector<BindShaderResourceFunc> >::iterator it = _textureFuncMap.find(binding);
		if (it != _textureFuncMap.end())
		{
			Vector<BindShaderResourceFunc> &funcs = (*it).second;
			for (uint i = 0; i < funcs.size(); i++)
			{
				(this->*funcs[i])(pSRV, binding);
			}
		}
	}

	void D3D11Shader::BindConstantBufferVS(ID3D11Buffer * pBuffer, uint binding, uint firstConstant, uint numConstants)
	{
		_device->VSSetConstantBuffer(binding, pBuffer, firstConstant, numConstants);
	}
	void D3D11Shader::BindConstantBufferPS(ID3D11Buffer * pBuffer, uint binding, uint firstConstant, uint numConstants)
	{
		_device->PSSetConstantBuffer(binding, pBuffer, firstConstant, numConstants);
	}
	void D3D11Shader::BindConstantBufferGS(ID3D11Buffer * pBuffer, uint binding, uint firstConstant, uint numConstants)
	{
		_device->GSSetConstantBuffer(binding, pBuffer, firstConstant, numConstants);
	}
	void D3D11Shader::BindSamplerVS(ID3D11SamplerState * pSampler, uint binding)
	{
		_device->VSSetSampler(binding, pSampler);
	}
	void D3D11Shader::BindSamplerPS(ID3D11SamplerState * pSampler, uint binding)
	{
		_device->PSSetSampler(binding, pSampler);
	}
	void D3D11Shader::BindSamplerGS(ID3D11SamplerState * pSampler, uint binding)
	{
		_device->GSSetSampler(binding, pSampler);
	}
	void D3D11Shader::BindShaderResourceVS(ID3D11ShaderResourceView * pShaderResourceView, uint binding)
	{
		_device->VSSetShaderResource(binding, pShaderResourceView);
	}
	void D3D11Shader::BindShaderResourcePS(ID3D11ShaderResourceView * pShaderResourceView, uint binding)
	{
		_device->PSSetShaderResource(binding, pShaderResourceView);
	}
	void D3D11Shader::BindShaderResourceGS(ID3D11ShaderResourceView * pShaderResourceView, uint binding)
	{
		_device->GSSetShaderResource(binding, pShaderResourceView);
	}

	D3D11ShaderBindings::D3D11ShaderBindings()
	{

	}

	D3D11ShaderBindings::~D3D11ShaderBindings()
	{

	}

	bool D3D11ShaderBindings::Create(const IShaderBindingCreateInfo& createInfo)
	{
		_bindingMap.clear();

		for (uint i = 0; i < createInfo.bufferBindings.size(); i++)
		{
			_bindingMap[createInfo.bufferBindings[i].name] = { createInfo.bufferBindings[i].binding[SE_GFX_D3D11], NULL };
		}

		for (uint i = 0; i < createInfo.resourceBindings.size(); i++)
		{
			_bindingMap[createInfo.resourceBindings[i].name] = { createInfo.resourceBindings[i].binding[SE_GFX_D3D11], NULL };
		}

		return true;
	}

	void D3D11ShaderBindings::Bind(ICommandBuffer *pCmdBuffer, IBindState* pBindState)
	{
		D3D11CommandBuffer* dxCmd = static_cast<D3D11CommandBuffer*>(pCmdBuffer);

		for (auto iter = _bindingMap.begin(); iter != _bindingMap.end(); ++iter)
		{
			D3D11ShaderResource* pRes = (*iter).second.second;
			pRes->BindToShader(dxCmd->GetCurrentShader(), (*iter).first, (*iter).second.first, pBindState);
		}
	}

	void D3D11ShaderBindings::Unbind(ICommandBuffer *pCmdBuffer)
	{
		(void)pCmdBuffer;
	}

	void D3D11ShaderBindings::SetTexture(ITexture* pTexture, const String& name)
	{
		_bindingMap.at(name).second = static_cast<D3D11Texture*>(pTexture);
	}

	void D3D11ShaderBindings::SetSampler(ISampler* pSampler, const String& name)
	{
		_bindingMap.at(name).second = static_cast<D3D11Sampler*>(pSampler);
	}

	void D3D11ShaderBindings::SetTextureCube(ITextureCube* pTextureCube, const String& name)
	{
		_bindingMap.at(name).second = static_cast<D3D11TextureCube*>(pTextureCube);
	}

	void D3D11ShaderBindings::SetTextureArray(ITextureArray * pTextureArray, const String& name)
	{
		_bindingMap.at(name).second = static_cast<D3D11TextureArray*>(pTextureArray);
	}

	void D3D11ShaderBindings::SetUniformBuffer(IUniformBuffer* pBuffer, const String& name)
	{
		_bindingMap.at(name).second = static_cast<D3D11UniformBuffer*>(pBuffer);
	}

}