#include "D3D11Shader.h"
#include "D3D11Texture.h"

namespace SunEngine
{

	D3D11Texture::D3D11Texture()
	{
		_texture = 0;
		_srv = 0;
	}


	D3D11Texture::~D3D11Texture()
	{
	}

	bool D3D11Texture::Create(const ITextureCreateInfo& info)
	{
		uint width = info.images[0].image.Width;
		uint height = info.images[0].image.Height;
		uint mips = info.images[0].mipLevels;
		uint flags = info.images[0].image.Flags;

		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.ArraySize = info.numImages;
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.MipLevels = 1 + mips;
		texDesc.SampleDesc.Count = 1;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (flags & ImageData::CUBEMAP) 
			texDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

		Vector<D3D11_SUBRESOURCE_DATA> subDataArray;
		for (uint i = 0; i < info.numImages; i++)
		{
			D3D11_SUBRESOURCE_DATA subData = {};
			subData.pSysMem = info.images[i].image.Pixels;
			subData.SysMemPitch = sizeof(Pixel) * width;
			subDataArray.push_back(subData);

			for (uint j = 0; j < info.images[i].mipLevels; j++)
			{
				subData.pSysMem = info.images[i].pMips[j].Pixels;
				subData.SysMemPitch = sizeof(Pixel) * info.images[i].pMips[j].Width;
				subDataArray.push_back(subData);
			}
		}

		D3D11_SUBRESOURCE_DATA* pSubData = subDataArray.data();
		if (flags & ImageData::DEPTH_BUFFER)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL;
			texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			pSubData = 0;
		}
		else if (flags & ImageData::COLOR_BUFFER_RGBA8)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			pSubData = 0;
		}
		else if (flags & ImageData::COLOR_BUFFER_RGBA16F)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
			texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			pSubData = 0;
		}
		else if (flags & ImageData::COMPRESSED_BC1)
		{
			texDesc.Format = DXGI_FORMAT_BC1_UNORM;
			for (uint i = 0; i < subDataArray.size(); i++)
			{
				subDataArray[i].SysMemPitch /= 2;
			}
		}
		else if (flags & ImageData::COMPRESSED_BC3)
		{
			texDesc.Format = DXGI_FORMAT_BC3_UNORM;
			for (uint i = 0; i < subDataArray.size(); i++)
			{
				subDataArray[i].SysMemPitch /= 1;
			}
		}
		else if (flags & ImageData::SAMPLED_TEXTURE_R32F)
		{
			texDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (flags & ImageData::SAMPLED_TEXTURE_R32G32B32A32F)
		{
			texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			for (uint i = 0; i < subDataArray.size(); i++)
			{
				subDataArray[i].SysMemPitch *= 4;
			}
		}

		if (flags & ImageData::SRGB)
		{
			if (texDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM) texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			else if (texDesc.Format == DXGI_FORMAT_BC1_UNORM) texDesc.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
			else if (texDesc.Format == DXGI_FORMAT_BC3_UNORM) texDesc.Format = DXGI_FORMAT_BC3_UNORM_SRGB;
		}

		if (flags & ImageData::MULTI_SAMPLES_2) _device->FillSampleDesc(texDesc.Format, 2, texDesc.SampleDesc);
		else if (flags & ImageData::MULTI_SAMPLES_4) _device->FillSampleDesc(texDesc.Format, 4, texDesc.SampleDesc);
		else if (flags & ImageData::MULTI_SAMPLES_8) _device->FillSampleDesc(texDesc.Format, 8, texDesc.SampleDesc);
		else texDesc.SampleDesc.Count = 1;

		if (!_device->CreateTexture2D(texDesc, pSubData, &_texture)) return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texDesc.Format;

		if (flags & ImageData::CUBEMAP)
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = 1 + mips;
		}
		else if (info.numImages > 1)
		{
			if (texDesc.SampleDesc.Count == 1)
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.ArraySize = info.numImages;
				srvDesc.Texture2DArray.MipLevels = 1 + mips;
			}
			else
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				srvDesc.Texture2DMSArray.ArraySize = info.numImages;
			}
		}
		else
		{
			if (texDesc.SampleDesc.Count == 1)
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1 + mips;
			}
			else
			{
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
			}
		}

		if (flags & ImageData::DEPTH_BUFFER)
		{
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		
		if (!_device->CreateShaderResourceView(_texture, srvDesc, &_srv)) return false;

		return true;
	}

	void D3D11Texture::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void D3D11Texture::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	void D3D11Texture::BindToShader(D3D11Shader* pShader, const String&, uint binding, IBindState*)
	{
		pShader->BindTexture(this, binding);
	}

	bool D3D11Texture::Destroy()
	{
		COM_RELEASE(_srv);
		COM_RELEASE(_texture);
		return true;
	}

}