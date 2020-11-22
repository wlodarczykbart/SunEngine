#include <assert.h>

#include "ConfigFile.h"
#include "StringUtil.h"
#include "FileReader.h"

#include "DirectXCommandBuffer.h"
#include "DirectXRenderTarget.h"
#include "DirectXSampler.h"
#include "DirectXUniformBuffer.h"
#include "DirectXTexture.h"
#include "DirectXTextureCube.h"
#include "DirectXTextureArray.h"

#include "Sampler.h"
#include "BaseTexture.h"
#include "GraphicsContext.h"

#include "MemBuffer.h"

#include "DirectXShader.h"

namespace SunEngine
{
	DirectXShader::DirectXShader()
	{
		_vertexShader = 0;
		_pixelShader = 0;
		_geometryShader = 0;
		_inputLayout = 0;
	}


	DirectXShader::~DirectXShader()
	{
		Destroy();
	}

	bool DirectXShader::Create(IShaderCreateInfo& info)
	{
		FileReader rdr;

		MemBuffer vertexBuffer;
		rdr.Open(info.vertexShaderFilename.data());
		rdr.ReadAll(vertexBuffer);
		rdr.Close();
		if (!_device->CreateVertexShader(vertexBuffer.GetData(), vertexBuffer.GetSize(), &_vertexShader)) return false;

		if (info.pixelShaderFilename.length())
		{
			MemBuffer pixelBuffer;
			rdr.Open(info.pixelShaderFilename.data());
			rdr.ReadAll(pixelBuffer);
			rdr.Close();
			if (!_device->CreatePixelShader(pixelBuffer.GetData(), pixelBuffer.GetSize(), &_pixelShader)) return false;
		}

		if (info.geometryShaderFilename.length())
		{
			MemBuffer geometryBuffer;
			rdr.Open(info.geometryShaderFilename.data());
			rdr.ReadAll(geometryBuffer);
			rdr.Close();
			if (!_device->CreateGeometryShader(geometryBuffer.GetData(), geometryBuffer.GetSize(), &_geometryShader)) return false;
		}

		Vector<D3D11_INPUT_ELEMENT_DESC> elements;
		for (uint i = 0; i < info.vertexElements.size(); i++)
		{
			APIVertexElement &data = info.vertexElements[i];

			D3D11_INPUT_ELEMENT_DESC elem = {};
			elem.AlignedByteOffset = data.offset;
			elem.SemanticName = data.semantic.data();
			elem.InstanceDataStepRate = D3D11_INPUT_PER_VERTEX_DATA;

			switch (data.format)
			{
			case APIVertexInputFormat::VIF_FLOAT2:
				elem.Format = DXGI_FORMAT_R32G32_FLOAT;
				break;
			case APIVertexInputFormat::VIF_FLOAT3:
				elem.Format = DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case APIVertexInputFormat::VIF_FLOAT4:
				elem.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			default:
				break;
			}
			elements.push_back(elem);
		}

		if (elements.size())
		{
			if (!_device->CreateInputLayout(elements.data(), (uint)elements.size(), vertexBuffer.GetData(), vertexBuffer.GetSize(), &_inputLayout)) return false;
		}

		for (uint i = 0; i < info.constBuffers.size(); i++)
		{
			IShaderResource& buff = info.constBuffers[i];

			Vector<BindConstantBufferFunc>& funcs = _constBufferFuncMap[buff.binding];
			if (buff.stages & SS_VERTEX)
				funcs.push_back(&DirectXShader::BindConstantBufferVS);
			if (buff.stages & SS_PIXEL)
				funcs.push_back(&DirectXShader::BindConstantBufferPS);
			if (buff.stages & SS_GEOMETRY)
				funcs.push_back(&DirectXShader::BindConstantBufferGS);
		}

		for (uint i = 0; i < info.textures.size(); i++)
		{
			IShaderResource& tex = info.textures[i];

			Vector<BindShaderResourceFunc>& funcs = _textureFuncMap[tex.binding];
			if (tex.stages & SS_VERTEX)
				funcs.push_back(&DirectXShader::BindShaderResourceVS);
			if (tex.stages & SS_PIXEL)
				funcs.push_back(&DirectXShader::BindShaderResourcePS);
			if (tex.stages & SS_GEOMETRY)
				funcs.push_back(&DirectXShader::BindShaderResourceGS);

			_textureFlagsMap[tex.binding] = tex.flags;
		}

		for (uint i = 0; i < info.samplers.size(); i++)
		{
			IShaderResource& sampler = info.samplers[i];

			Vector<BindSamplerFunc>& funcs = _samplerFuncMap[sampler.binding];
			if (sampler.stages & SS_VERTEX)
				funcs.push_back(&DirectXShader::BindSamplerVS);
			if (sampler.stages & SS_PIXEL)
				funcs.push_back(&DirectXShader::BindSamplerPS);
			if (sampler.stages & SS_GEOMETRY)
				funcs.push_back(&DirectXShader::BindSamplerPS);
		}

		return true;
	}

	bool DirectXShader::Destroy()
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

	void DirectXShader::Bind(ICommandBuffer * cmdBuffer)
	{
		DirectXCommandBuffer* dxCmd = (DirectXCommandBuffer*)cmdBuffer;
		dxCmd->SetCurrentShader(this);

		if(_inputLayout) dxCmd->BindInputLayout(_inputLayout);
		if(_vertexShader) dxCmd->BindVertexShader(_vertexShader);
		if(_pixelShader) dxCmd->BindPixelShader(_pixelShader);
		if(_geometryShader) dxCmd->BindGeometryShader(_geometryShader);
	}

	void DirectXShader::Unbind(ICommandBuffer * cmdBuffer)
	{
		DirectXCommandBuffer* dxCmd = (DirectXCommandBuffer*)cmdBuffer;
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
				(this->*funcs[i])(pNullBuffer, binding);
			}
			bufferIt++;
		}

		dxCmd->SetCurrentShader(0);
	}

	void DirectXShader::BindTexture(DirectXTexture * pTexture, uint binding)
	{
		BindShaderResourceView(pTexture->_srv, binding);
	}

	void DirectXShader::BindTextureCube(DirectXTextureCube * pTextureCube, uint binding)
	{
		BindShaderResourceView(pTextureCube->_srv, binding);
	}

	void DirectXShader::BindTextureArray(DirectXTextureArray * pTextureArray, uint binding)
	{
		BindShaderResourceView(pTextureArray->_srv, binding);
	}

	void DirectXShader::BindSampler(DirectXSampler * pSampler, uint binding)
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

	uint DirectXShader::GetTextureType(uint binding) const
	{
		Map<uint, uint>::const_iterator iter = _textureFlagsMap.find(binding);

		if (iter == _textureFlagsMap.end())
			return 0;

		uint flags = (*iter).second;

		if (flags & SRF_TEXTURE_2D)
			return SRF_TEXTURE_2D;

		if (flags & SRF_TEXTURE_CUBE)
			return SRF_TEXTURE_CUBE;

		if (flags & SRF_TEXTURE_ARRAY)
			return SRF_TEXTURE_ARRAY;

		return 0;
	}

	void DirectXShader::BindShaderResourceView(ID3D11ShaderResourceView * pSRV, uint binding)
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

	void DirectXShader::BindConstantBufferVS(ID3D11Buffer * pBuffer, uint binding)
	{
		_device->VSSetConstantBuffers(binding, 1, &pBuffer);
	}
	void DirectXShader::BindConstantBufferPS(ID3D11Buffer * pBuffer, uint binding)
	{
		_device->PSSetConstantBuffers(binding, 1, &pBuffer);
	}
	void DirectXShader::BindConstantBufferGS(ID3D11Buffer * pBuffer, uint binding)
	{
		_device->GSSetConstantBuffers(binding, 1, &pBuffer);
	}
	void DirectXShader::BindSamplerVS(ID3D11SamplerState * pSampler, uint binding)
	{
		_device->VSSetSamplers(binding, 1, &pSampler);
	}
	void DirectXShader::BindSamplerPS(ID3D11SamplerState * pSampler, uint binding)
	{
		_device->PSSetSamplers(binding, 1, &pSampler);
	}
	void DirectXShader::BindSamplerGS(ID3D11SamplerState * pSampler, uint binding)
	{
		_device->GSSetSamplers(binding, 1, &pSampler);
	}
	void DirectXShader::BindShaderResourceVS(ID3D11ShaderResourceView * pShaderResourceView, uint binding)
	{
		_device->VSSetShaderResources(binding, 1, &pShaderResourceView);
	}
	void DirectXShader::BindShaderResourcePS(ID3D11ShaderResourceView * pShaderResourceView, uint binding)
	{
		_device->PSSetShaderResources(binding, 1, &pShaderResourceView);
	}
	void DirectXShader::BindShaderResourceGS(ID3D11ShaderResourceView * pShaderResourceView, uint binding)
	{
		_device->GSSetShaderResources(binding, 1, &pShaderResourceView);
	}

	DirectXShader::ConstBufferData::ConstBufferData()
	{
		pBuffer = new DirectXUniformBuffer();
	}

	DirectXShaderBindings::DirectXShaderBindings()
	{

	}

	DirectXShaderBindings::~DirectXShaderBindings()
	{

	}

	//bool DirectXShaderBindings::Create(IShader* pShader)
	//{
	//	_shader = static_cast<DirectXShader*>(pShader);
	//	_textureBindings.clear();
	//	_samplerBindings.clear();

	//	Vector<uint> textures, samplers;
	//	_shader->GetShaderResourceBindings(textures, samplers);

	//	for(uint i = 0; i < textures.size(); i++)
	//	{
	//		_textureBindings[textures[i]] = NULL;
	//	}

	//	for (uint i = 0; i < samplers.size(); i++)
	//	{
	//		_samplerBindings[samplers[i]] = NULL;
	//	}

	//	return true;
	//}

	bool DirectXShaderBindings::Create(IShader* pShader, const Vector<IShaderResource>& textureBindings, const Vector<IShaderResource>& samplerBindings)
	{
		_textureBindings.clear();
		_samplerBindings.clear();

		for(uint i = 0; i < textureBindings.size(); i++)
		{
			_textureBindings[textureBindings[i].binding] = NULL;
		}

		for (uint i = 0; i < samplerBindings.size(); i++)
		{
			_samplerBindings[samplerBindings[i].binding] = NULL;
		}

		return true;
	}

	void DirectXShaderBindings::Bind(ICommandBuffer *pCmdBuffer)
	{
		DirectXCommandBuffer* dxCmd = static_cast<DirectXCommandBuffer*>(pCmdBuffer);

		Map<uint, DirectXShaderResource*>::iterator samplerIter = _samplerBindings.begin();
		while (samplerIter != _samplerBindings.end())
		{
			DirectXShaderResource* pRes = (*samplerIter).second;
			if (!pRes)
				pRes = static_cast<DirectXSampler*>(GraphicsContext::GetDefaultSampler(GraphicsContext::DS_LINEAR_REPEAT)->GetAPIHandle());

			pRes->BindToShader(dxCmd->GetCurrentShader(), (*samplerIter).first);
			++samplerIter;
		}

		Map<uint, DirectXShaderResource*>::iterator textureIter = _textureBindings.begin();
		while (textureIter != _textureBindings.end())
		{
			DirectXShaderResource* pRes = (*textureIter).second;
			if (!pRes)
			{
				uint type = dxCmd->GetCurrentShader()->GetTextureType((*textureIter).first);
				if (type == SRF_TEXTURE_2D)
				{
					pRes = static_cast<DirectXTexture*>(GraphicsContext::GetDefaultTexture(GraphicsContext::DT_WHITE)->GetAPIHandle());
				}
				else if (type == SRF_TEXTURE_CUBE)
				{
					//TODO...
				}
			}

			pRes->BindToShader(dxCmd->GetCurrentShader(), (*textureIter).first);
			++textureIter;
		}
	}

	void DirectXShaderBindings::Unbind(ICommandBuffer *pCmdBuffer)
	{
		(void)pCmdBuffer;
	}

	void DirectXShaderBindings::SetTexture(ITexture* pTexture, uint binding)
	{
		_textureBindings[binding] = static_cast<DirectXTexture*>(pTexture);
	}

	void DirectXShaderBindings::SetSampler(ISampler* pSampler, uint binding)
	{
		_samplerBindings[binding] = static_cast<DirectXSampler*>(pSampler);
	}

	void DirectXShaderBindings::SetTextureCube(ITextureCube* pTextureCube, uint binding)
	{
		_textureBindings[binding] = static_cast<DirectXTextureCube*>(pTextureCube);
	}

	void DirectXShaderBindings::SetTextureArray(ITextureArray * pTextureArray, uint binding)
	{
		_textureBindings[binding] = static_cast<DirectXTextureArray*>(pTextureArray);
	}

}