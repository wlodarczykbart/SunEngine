#include "D3D11Shader.h"
#include "D3D11Texture.h"

namespace SunEngine
{
	static const Map<D3D11_SRV_DIMENSION, D3D11_SRV_DIMENSION> SRVDimensionMap =
	{
		{ D3D11_SRV_DIMENSION_TEXTURE2D, D3D11_SRV_DIMENSION_TEXTURE2D },
		{ D3D11_SRV_DIMENSION_TEXTURE2DARRAY, D3D11_SRV_DIMENSION_TEXTURE2DARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURE2DMS, D3D11_SRV_DIMENSION_TEXTURE2DMS },
		{ D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY, D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURECUBE,  D3D11_SRV_DIMENSION_TEXTURE2DARRAY },
		{ D3D11_SRV_DIMENSION_TEXTURECUBEARRAY,  D3D11_SRV_DIMENSION_TEXTURE2DARRAY },
	};

	D3D11Texture::D3D11Texture()
	{
		_texture = 0;
		_srv = 0;
		_cubeToArraySRV = 0;
		_uav = 0;
		_external = false;
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
			if (subData.pSysMem)
			{
				subData.SysMemPitch = sizeof(Pixel) * width;
				subDataArray.push_back(subData);
				for (uint j = 0; j < info.images[i].mipLevels; j++)
				{
					subData.pSysMem = info.images[i].pMips[j].Pixels;
					subData.SysMemPitch = sizeof(Pixel) * info.images[i].pMips[j].Width;
					subDataArray.push_back(subData);
				}
			}
		}

		if (flags & ImageData::DEPTH_BUFFER)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL;
			texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		}
		else if (flags & ImageData::COLOR_BUFFER_RGBA8)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
			texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		else if (flags & ImageData::COLOR_BUFFER_RGBA16F)
		{
			texDesc.BindFlags |= D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
			texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
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

		if (flags & ImageData::WRITABLE)
			texDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

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

		if (!_device->CreateTexture2D(texDesc, subDataArray.data(), &_texture)) return false;

		_viewDesc = {};
		_viewDesc.Format = texDesc.Format;

		if (flags & ImageData::CUBEMAP)
		{
			if (texDesc.ArraySize > 6)
			{
				_viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
				_viewDesc.TextureCubeArray.NumCubes = texDesc.ArraySize / 6;
				_viewDesc.TextureCubeArray.MipLevels = 1 + mips;
			}
			else
			{
				_viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				_viewDesc.TextureCube.MipLevels = 1 + mips;
			}
		}
		else if (info.numImages > 1)
		{
			if (texDesc.SampleDesc.Count == 1)
			{
				_viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				_viewDesc.Texture2DArray.ArraySize = info.numImages;
				_viewDesc.Texture2DArray.MipLevels = 1 + mips;
			}
			else
			{
				_viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
				_viewDesc.Texture2DMSArray.ArraySize = info.numImages;
			}
		}
		else
		{
			if (texDesc.SampleDesc.Count == 1)
			{
				_viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				_viewDesc.Texture2D.MipLevels = 1 + mips;
			}
			else
			{
				_viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
			}
		}

		if (flags & ImageData::DEPTH_BUFFER)
		{
			_viewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		
		if (!_device->CreateShaderResourceView(_texture, _viewDesc, &_srv)) return false;

		if (flags & ImageData::CUBEMAP)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC cubeToArrayDesc = _viewDesc;
			cubeToArrayDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			cubeToArrayDesc.Texture2DArray.ArraySize = info.numImages;
			cubeToArrayDesc.Texture2DArray.MipLevels = 1 + mips;
			if (!_device->CreateShaderResourceView(_texture, cubeToArrayDesc, &_cubeToArraySRV)) return false;
		}

		if (flags & ImageData::WRITABLE)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			if (info.numImages > 1)
			{
				uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize = info.numImages;
			}
			else
			{
				uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			}

			if (!_device->CreateUnorderedAccessView(_texture, uavDesc, &_uav)) return false;
		}

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

	void D3D11Texture::BindToShader(D3D11CommandBuffer* cmdBuffer, D3D11Shader* pShader, const String& name, uint binding, IBindState*)
	{
		pShader->BindTexture(cmdBuffer, this, name, binding);
	}

	bool D3D11Texture::Destroy()
	{
		COM_RELEASE(_srv);
		COM_RELEASE(_cubeToArraySRV);
		COM_RELEASE(_uav);
		if(!_external)
			COM_RELEASE(_texture);
		_external = false;
		return true;
	}

}