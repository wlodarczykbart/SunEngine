#pragma once

#include "Image.h"

namespace SunEngine
{
	class MipMapGenerator
	{
	public:
		MipMapGenerator();
		~MipMapGenerator();

		bool Create(const ImageData &baseImage);

		uint GetMipLevels() const;
		ImageData* GetMipMaps() const;

	private:
		static const int MIN_MIP_SIZE = 8;

		Vector<ImageData> _mipMaps;

	};
}
