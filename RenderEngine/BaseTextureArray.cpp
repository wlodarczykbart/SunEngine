#include "ITextureArray.h"
#include "BaseTextureArray.h"

namespace SunEngine
{

	BaseTextureArray::BaseTextureArray() : GraphicsObject(GraphicsObject::TEXTURE_ARRAY)
	{
		_iTexture = 0;
		_width = 0;
		_height = 0;
	}


	BaseTextureArray::~BaseTextureArray()
	{
	}

	bool BaseTextureArray::Create(const CreateInfo & info)
	{
		if (info.numImages == 0)
			return false;

		if (!Destroy())
			return false;

		if (!_iTexture)
			_iTexture = AllocateGraphics<ITextureArray>();

		ITextureArrayCreateInfo apiInfo = {};

		Vector<ITextureCreateInfo> apiImages;

		uint width = info.pImages[0].image.Width;
		uint height = info.pImages[0].image.Height;
		uint mips = info.pImages[0].mipLevels;
		for (uint i = 0; i < info.numImages; i++)
		{
			if (width != info.pImages[i].image.Width || height != info.pImages[i].image.Height || mips != info.pImages[i].mipLevels)
				return false;

			ITextureCreateInfo textureInfo;
			textureInfo.image = info.pImages[i].image;
			textureInfo.pMips = info.pImages[i].pMips;
			textureInfo.mipLevels = info.pImages[i].mipLevels;
			apiImages.push_back(textureInfo);
		}

		apiInfo.numImages = apiImages.size();
		apiInfo.pImages = apiImages.data();

		if (!_iTexture->Create(apiInfo))
		{
			_errStr = "Failed to create API Texture";
			return false;
		}

		_width = width;
		_height = height;
 
		return true;
	}

	bool BaseTextureArray::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_iTexture = 0;
		return true;
	}

	IObject * BaseTextureArray::GetAPIHandle() const
	{
		return _iTexture;
	}

	uint BaseTextureArray::GetWidth() const
	{
		return _width;
	}

	uint BaseTextureArray::GetHeight() const
	{
		return _height;
	}
}