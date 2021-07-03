#include <assert.h>

#include "ConfigFile.h"
#include "StringUtil.h"
#include "FileReader.h"

#include "D3D11CommandBuffer.h"
#include "D3D11RenderTarget.h"
#include "D3D11Sampler.h"
#include "D3D11UniformBuffer.h"
#include "D3D11Texture.h"

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
		_computeShader = 0;
		_inputLayout = 0;
	}


	D3D11Shader::~D3D11Shader()
	{
		Destroy();
	}

	bool D3D11Shader::Create(IShaderCreateInfo& info)
	{
		if (info.vertexBinaries[SE_GFX_D3D11].GetSize())
		{
		if (!_device->CreateVertexShader(info.vertexBinaries[SE_GFX_D3D11].GetData(), info.vertexBinaries[SE_GFX_D3D11].GetSize(), &_vertexShader)) return false;
		}

		if (info.pixelBinaries[SE_GFX_D3D11].GetSize())
		{
			if (!_device->CreatePixelShader(info.pixelBinaries[SE_GFX_D3D11].GetData(), info.pixelBinaries[SE_GFX_D3D11].GetSize(), &_pixelShader)) return false;
		}

		if (info.geometryBinaries[SE_GFX_D3D11].GetSize())
		{
			if (!_device->CreateGeometryShader(info.geometryBinaries[SE_GFX_D3D11].GetData(), info.geometryBinaries[SE_GFX_D3D11].GetSize(), &_geometryShader)) return false;
		}

		if (info.computeBinaries[SE_GFX_D3D11].GetSize())
		{
			if (!_device->CreateComputeShader(info.computeBinaries[SE_GFX_D3D11].GetData(), info.computeBinaries[SE_GFX_D3D11].GetSize(), &_computeShader)) return false;
		}

		Vector<D3D11_INPUT_ELEMENT_DESC> elements;
		for (uint i = 0; i < info.vertexElements.size(); i++)
		{
			IVertexElement& data = info.vertexElements[i];

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


		_resourceMap = info.resources;
		return true;
	}

	bool D3D11Shader::Destroy()
	{
		COM_RELEASE(_vertexShader);
		COM_RELEASE(_pixelShader);
		COM_RELEASE(_geometryShader);
		COM_RELEASE(_computeShader);
		COM_RELEASE(_inputLayout);
		_resourceMap.clear();

		return true;
	}

	void D3D11Shader::Bind(ICommandBuffer* cmdBuffer, IBindState*)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;
		dxCmd->SetCurrentShader(this);

		dxCmd->BindInputLayout(_inputLayout);
		dxCmd->BindVertexShader(_vertexShader);
		dxCmd->BindPixelShader(_pixelShader);
		dxCmd->BindGeometryShader(_geometryShader);
		dxCmd->BindComputeShader(_computeShader);
	}

	void D3D11Shader::Unbind(ICommandBuffer* cmdBuffer)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;
		dxCmd->BindInputLayout(0);
		dxCmd->BindVertexShader(0);
		dxCmd->BindPixelShader(0);
		dxCmd->BindGeometryShader(0);
		dxCmd->BindComputeShader(0);

		ID3D11ShaderResourceView* pNullSRV = 0;
		ID3D11SamplerState* pNullSampler = 0;
		ID3D11Buffer* pNullBuffer = 0;
		ID3D11UnorderedAccessView* pNullUAV = 0;

		for (auto iter = _resourceMap.begin(); iter != _resourceMap.end(); ++iter)
		{
			auto& res = (*iter).second;
			switch (res.type)
			{
			case SRT_TEXTURE:
				if (res.texture.readOnly)
					dxCmd->SetShaderResourceView(res.stages, res.binding[SE_GFX_D3D11], pNullSRV);
				else
					dxCmd->SetUnorderedAccessView(res.stages, res.binding[SE_GFX_D3D11], pNullUAV);
				break;
			case SRT_SAMPLER:
				dxCmd->SetSampler(res.stages, res.binding[SE_GFX_D3D11], pNullSampler);
				break;
			case SRT_BUFFER:
				dxCmd->SetConstantBuffer(res.stages, res.binding[SE_GFX_D3D11], pNullBuffer, 0, 0);
				break;
			default:
				break;
			}
		}

		dxCmd->SetCurrentShader(0);
	}

	void D3D11Shader::BindTexture(D3D11CommandBuffer* cmdBuffer, D3D11Texture * pTexture, const String& name, uint binding)
	{
		IShaderResource& res = _resourceMap.at(name);

		bool cubemapToArray =
			(pTexture->GetViewDimension() == D3D11_SRV_DIMENSION_TEXTURECUBE || pTexture->GetViewDimension() == D3D11_SRV_DIMENSION_TEXTURECUBEARRAY)
			&& res.texture.dimension == SRD_TEXTURE2DARRAY;

		if (res.texture.readOnly)
			cmdBuffer->SetShaderResourceView(res.stages, res.binding[SE_GFX_D3D11], cubemapToArray ? pTexture->_cubeToArraySRV : pTexture->_srv);
		else
			cmdBuffer->SetUnorderedAccessView(res.stages, res.binding[SE_GFX_D3D11], pTexture->_uav);
	}

	void D3D11Shader::BindSampler(D3D11CommandBuffer* cmdBuffer, D3D11Sampler * pSampler, const String& name, uint binding)
	{
		IShaderResource& res = _resourceMap.at(name);
		cmdBuffer->SetSampler(res.stages, res.binding[SE_GFX_D3D11], pSampler->_sampler);
	}

	void D3D11Shader::BindBuffer(D3D11CommandBuffer* cmdBuffer, D3D11UniformBuffer* pBuffer, const String& name, uint binding, uint firstConstant, uint numConstants)
	{
		IShaderResource& res = _resourceMap.at(name);
		cmdBuffer->SetConstantBuffer(res.stages, res.binding[SE_GFX_D3D11], pBuffer->_buffer, firstConstant, numConstants);
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
		for (uint i = 0; i < createInfo.resourceBindings.size(); i++)
			_bindingMap[createInfo.resourceBindings[i].name] = { createInfo.resourceBindings[i].binding[SE_GFX_D3D11], NULL };

		return true;
	}

	void D3D11ShaderBindings::Bind(ICommandBuffer *pCmdBuffer, IBindState* pBindState)
	{
		D3D11CommandBuffer* dxCmd = static_cast<D3D11CommandBuffer*>(pCmdBuffer);

		for (auto iter = _bindingMap.begin(); iter != _bindingMap.end(); ++iter)
		{
			D3D11ShaderResource* pRes = (*iter).second.second;
			if(pRes)
				pRes->BindToShader(dxCmd, dxCmd->GetCurrentShader(), (*iter).first, (*iter).second.first, pBindState);
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

	void D3D11ShaderBindings::SetUniformBuffer(IUniformBuffer* pBuffer, const String& name)
	{
		_bindingMap.at(name).second = static_cast<D3D11UniformBuffer*>(pBuffer);
	}

}