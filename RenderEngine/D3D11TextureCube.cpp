#include "D3D11Shader.h"
#include "D3D11TextureCube.h"

namespace SunEngine
{

	D3D11TextureCube::D3D11TextureCube()
	{
		_texture = 0;
		_srv = 0;
	}


	D3D11TextureCube::~D3D11TextureCube()
	{
	}

	bool D3D11TextureCube::Create(const ITextureCubeCreateInfo & info)
	{
		uint width = info.images[0].Width;
		uint height = info.images[0].Height;

		for (uint i = 1; i < 6; i++)
		{
			if (info.images[i].Width != width || info.images[i].Height != height)
				return false;
		}

		D3D11_TEXTURE2D_DESC desc = {};
		desc.ArraySize = 6;
		desc.Width = width;
		desc.Height = height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.MipLevels = 1;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
		desc.SampleDesc.Count = 1;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		Vector<D3D11_SUBRESOURCE_DATA> subDataArray;
		for (uint i = 0; i < 6; i++)
		{
			D3D11_SUBRESOURCE_DATA subData = {};
			subData.pSysMem = info.images[i].Pixels;
			subData.SysMemPitch = sizeof(Pixel) * width;
			subDataArray.push_back(subData);
		}

		if (!_device->CreateTexture2D(desc, subDataArray.data(), &_texture)) return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Format = desc.Format;
		
		if (!_device->CreateShaderResourceView(_texture, srvDesc, &_srv)) return false;

		return true;
	}

	void D3D11TextureCube::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		(void)cmdBuffer;
	}

	void D3D11TextureCube::Unbind(ICommandBuffer * cmdBuffer)
	{
		(void)cmdBuffer;
	}

	void D3D11TextureCube::BindToShader(D3D11Shader* pShader, const String&, uint binding, IBindState*)
	{
		pShader->BindTextureCube(this, binding);
	}

	bool D3D11TextureCube::Destroy()
	{
		COM_RELEASE(_srv);
		COM_RELEASE(_texture);
		return true;
	}

}