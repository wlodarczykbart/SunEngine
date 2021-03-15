#include "D3D11Shader.h"
#include "D3D11Texture.h"


namespace SunEngine
{

	D3D11Texture::D3D11Texture()
	{
	}


	D3D11Texture::~D3D11Texture()
	{
		Destroy();
	}

	bool D3D11Texture::Create(const ITextureCreateInfo & info)
	{
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.ArraySize = 1;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.Width = info.image.Width;
		texDesc.Height = info.image.Height;
		texDesc.MipLevels = 1 + info.mipLevels;
		texDesc.SampleDesc.Count = 1;
		
		Vector<D3D11_SUBRESOURCE_DATA> subData;

		D3D11_SUBRESOURCE_DATA imgData = {};
		imgData.pSysMem = info.image.Pixels;
		imgData.SysMemPitch = sizeof(Pixel) * info.image.Width;
		subData.push_back(imgData);

		for (uint i = 0; i < info.mipLevels; i++)
		{
			imgData.pSysMem = info.pMips[i].Pixels;
			imgData.SysMemPitch = sizeof(Pixel) * info.pMips[i].Width;
			subData.push_back(imgData);
		}

		if (info.image.Flags & ImageData::DEPTH_BUFFER)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL;
			texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			subData.clear();
		}
		else if (info.image.Flags & ImageData::COLOR_BUFFER_RGBA8)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			subData.clear();
		}
		else if (info.image.Flags & ImageData::COLOR_BUFFER_RGBA16F)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
			texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			subData.clear();
		}
		else if (info.image.Flags & ImageData::COMPRESSED_BC1)
		{
			texDesc.Format = DXGI_FORMAT_BC1_UNORM;
			for (uint i = 0; i < subData.size(); i++)
			{
				uint width = i == 0 ? info.image.Width : info.pMips[i - 1].Width;
				subData[i].SysMemPitch = sizeof(Pixel) * (width / 2);
			}
		}
		else if (info.image.Flags & ImageData::COMPRESSED_BC3)
		{
			texDesc.Format = DXGI_FORMAT_BC3_UNORM;
			for (uint i = 0; i < subData.size(); i++)
			{
				uint width = i == 0 ? info.image.Width : info.pMips[i - 1].Width;
				subData[i].SysMemPitch = sizeof(Pixel) * (width / 1);
			}
		}
		else if (info.image.Flags & ImageData::SAMPLED_TEXTURE_R32F)
		{
			texDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (info.image.Flags & ImageData::SAMPLED_TEXTURE_R32G32B32A32F)
		{
			texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			for (uint i = 0; i < subData.size(); i++)
			{
				uint width = i == 0 ? info.image.Width : info.pMips[i - 1].Width;
				subData[i].SysMemPitch = sizeof(float) * 4 * width;
			}
		}

		if (info.image.Flags & ImageData::SRGB)
		{
			if (texDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM) texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			else if (texDesc.Format == DXGI_FORMAT_BC1_UNORM) texDesc.Format = DXGI_FORMAT_BC1_UNORM_SRGB;
			else if (texDesc.Format == DXGI_FORMAT_BC3_UNORM) texDesc.Format = DXGI_FORMAT_BC3_UNORM_SRGB;
		}

		D3D11_SUBRESOURCE_DATA* pData = subData.size() ? subData.data() : 0;
		if (!_device->CreateTexture2D(texDesc, pData, &_texture)) return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Texture2D.MipLevels = (UINT)-1;
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

		if (info.image.Flags & ImageData::DEPTH_BUFFER)
		{
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}

		if (!_device->CreateShaderResourceView(_texture, srvDesc, &_srv)) return false;

		_format = srvDesc.Format;
		return true;
	}

	void D3D11Texture::Bind(ICommandBuffer *, IBindState*)
	{
	}

	void D3D11Texture::Unbind(ICommandBuffer *)
	{
	}

	void D3D11Texture::BindToShader(D3D11Shader* pShader, const String&, uint binding, IBindState*)
	{
		pShader->BindTexture(this, binding);
	}

	bool D3D11Texture::Destroy()
	{
		COM_RELEASE(_texture);
		COM_RELEASE(_srv);

		return true;
	}
}