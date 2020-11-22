#include "ITextureCube.h"
#include "BaseTextureCube.h"

namespace SunEngine
{

	BaseTextureCube::BaseTextureCube() : GraphicsObject(GraphicsObject::TEXTURE_CUBE)
	{
		_iTexture = 0;
		_width = 0;
		_height = 0;
	}


	BaseTextureCube::~BaseTextureCube()
	{
	}

	bool BaseTextureCube::Create(const CreateInfo & info)
	{
		if (!_iTexture)
			_iTexture = AllocateGraphics<ITextureCube>();

		ITextureCubeCreateInfo apiInfo = {};
		memcpy(apiInfo.images, info.images, sizeof(info.images));

		if (!_iTexture->Create(apiInfo))
		{
			_errStr = "Failed to create API Texture";
			return false;
		}

		_width = info.images[0].Width;
		_height = info.images[0].Height;
 
		return true;
	}

	bool BaseTextureCube::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_iTexture = 0;
		return true;
	}

	IObject * BaseTextureCube::GetAPIHandle() const
	{
		return _iTexture;
	}

	uint BaseTextureCube::GetWidth() const
	{
		return _width;
	}

	uint BaseTextureCube::GetHeight() const
	{
		return _height;
	}
}