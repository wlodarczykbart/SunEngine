#include "DirectXShader.h"
#include "DirectXTextureArray.h"

namespace SunEngine
{

	DirectXTextureArray::DirectXTextureArray()
	{
		_texture = 0;
		_srv = 0;
	}


	DirectXTextureArray::~DirectXTextureArray()
	{
	}

	bool DirectXTextureArray::Create(const ITextureArrayCreateInfo & info)
	{
		uint width = info.pImages[0].image.Width;
		uint height = info.pImages[0].image.Height;
		uint mips = info.pImages[0].mipLevels;
		uint flags = info.pImages[0].image.Flags;

		D3D11_TEXTURE2D_DESC desc = {};
		desc.ArraySize = info.numImages;
		desc.Width = width;
		desc.Height = height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.MipLevels = 1 + mips;
		desc.SampleDesc.Count = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		if (flags & ImageData::SAMPLED_TEXTURE_R32F)
		{
			desc.Format = DXGI_FORMAT_R32_FLOAT;
		}

		Vector<D3D11_SUBRESOURCE_DATA> subDataArray;

		for (uint i = 0; i < info.numImages; i++)
		{
			D3D11_SUBRESOURCE_DATA subData = {};
			subData.pSysMem = info.pImages[i].image.Pixels;
			subData.SysMemPitch = sizeof(Pixel) * width;
			subDataArray.push_back(subData);

			for (uint j = 0; j < info.pImages[i].mipLevels; j++)
			{
				subData.pSysMem = info.pImages[i].pMips[j].Pixels;
				subData.SysMemPitch = sizeof(Pixel) * info.pImages[i].pMips[j].Width;
				subDataArray.push_back(subData);
			}
		}

		if (!_device->CreateTexture2D(desc, subDataArray.data(), &_texture)) return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Texture2DArray.ArraySize = info.numImages;
		srvDesc.Texture2DArray.MipLevels = 1 + mips;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Format = desc.Format;
		
		if (!_device->CreateShaderResourceView(_texture, srvDesc, &_srv)) return false;

		return true;
	}

	void DirectXTextureArray::Bind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	void DirectXTextureArray::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	void DirectXTextureArray::BindToShader(DirectXShader * pShader, uint binding)
	{
		pShader->BindTextureArray(this, binding);
	}

	bool DirectXTextureArray::Destroy()
	{
		COM_RELEASE(_srv);
		COM_RELEASE(_texture);
		return true;
	}

}