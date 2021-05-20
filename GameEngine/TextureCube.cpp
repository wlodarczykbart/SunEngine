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

		BaseTextureCube::CreateInfo info = {};
		for (uint i = 0; i < 6; i++)
			info.images[i] = _images[i].ImageData();

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