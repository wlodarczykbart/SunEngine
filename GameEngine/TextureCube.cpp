#include "MipMapGenerator.h"
#include "TextureCube.h"

namespace SunEngine
{
	TextureCube::TextureCube()
	{
	}

	TextureCube::~TextureCube()
	{
	}

	bool TextureCube::RegisterToGPU()
	{
		if (!_gpuObject.Destroy())
			return false;

		BaseTexture::CreateInfo info = {};
		BaseTexture::CreateInfo::TextureData texData[6];
		for (uint i = 0; i < 6; i++)
		{
			texData[i] = {};
			texData[i].image = _images[i].ImageData();
			texData[i].image.Flags |= ImageData::CUBEMAP;
		}

		info.numImages = 6;
		info.pImages = texData;
		if (!_gpuObject.Create(info))
			return false;

		return true;
	}

	bool TextureCube::LoadFromFile(const String& path, const Vector<String>& sideNames)
	{
		if (sideNames.size() != 6)
			return false;

		for (uint i = 0; i < sideNames.size(); i++)
		{
			String sidePath = path + sideNames[i];
			if (!_images[i].Load(sidePath))
			{
				return false;
			}
		}
		return true;
	}

}