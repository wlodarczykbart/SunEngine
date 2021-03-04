#include "D3D11Shader.h"
#include "D3D11TextureArray.h"

namespace SunEngine
{

	D3D11TextureArray::D3D11TextureArray()
	{
		_texture = 0;
		_srv = 0;
	}


	D3D11TextureArray::~D3D11TextureArray()
	{
	}

	bool D3D11TextureArray::Create(const ITextureArrayCreateInfo & info)
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

	void D3D11TextureArray::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void D3D11TextureArray::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	void D3D11TextureArray::BindToShader(D3D11Shader* pShader, const String&, uint binding, IBindState*)
	{
		pShader->BindTextureArray(this, binding);
	}

	bool D3D11TextureArray::Destroy()
	{
		COM_RELEASE(_srv);
		COM_RELEASE(_texture);
		return true;
	}

}