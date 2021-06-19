#include "ITexture.h"
#include "BaseTexture.h"

namespace SunEngine
{
	BaseTexture::CreateInfo::CreateInfo()
	{
		pImages = 0;
		numImages = 0;
		isExternal = false;
	}

	BaseTexture::BaseTexture() : GraphicsObject(GraphicsObject::TEXTURE_ARRAY)
	{
		_iTexture = 0;
		_width = 0;
		_height = 0;
	}


	BaseTexture::~BaseTexture()
	{
	}

	bool BaseTexture::Create(const CreateInfo& info)
	{
		if (!Destroy())
			return false;

		if (info.numImages == 0)
			return false;

		if (!_iTexture)
			_iTexture = AllocateGraphics<ITexture>();

		uint width = info.pImages[0].image.Width;
		uint height = info.pImages[0].image.Height;

		if (!info.isExternal)
		{
			ITextureCreateInfo apiInfo = {};
			Vector<ITextureCreateInfo::TextureData> apiImages;

			uint mips = info.pImages[0].mipLevels;
			for (uint i = 0; i < info.numImages; i++)
			{
				if (width != info.pImages[i].image.Width || height != info.pImages[i].image.Height || mips != info.pImages[i].mipLevels)
					return false;

				ITextureCreateInfo::TextureData textureInfo;
				textureInfo.image = info.pImages[i].image;
				textureInfo.pMips = info.pImages[i].pMips;
				textureInfo.mipLevels = info.pImages[i].mipLevels;
				apiImages.push_back(textureInfo);
			}

			apiInfo.numImages = apiImages.size();
			apiInfo.images = apiImages.data();

			if (!_iTexture->Create(apiInfo))
			{
				_errStr = "Failed to create API Texture";
				return false;
			}
		}

		_width = width;
		_height = height;

		return true;
	}

	bool BaseTexture::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_iTexture = 0;
		return true;
	}

	IObject* BaseTexture::GetAPIHandle() const
	{
		return _iTexture;
	}

	uint BaseTexture::GetWidth() const
	{
		return _width;
	}

	uint BaseTexture::GetHeight() const
	{
		return _height;
	}
}