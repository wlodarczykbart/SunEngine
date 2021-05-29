#pragma once

#include "BaseTextureCube.h"
#include "GPUResource.h"
#include "Image.h"

namespace SunEngine
{
	class TextureCube : public GPUResource<BaseTextureCube>
	{
	public:
		TextureCube();
		~TextureCube();

		bool RegisterToGPU() override;
		bool LoadFromFile(const String& path, const Vector<String>& sideNames);
	private:
		Image _images[6];
	};
}