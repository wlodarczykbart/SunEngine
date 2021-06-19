#pragma once

#include "BaseTexture.h"
#include "GPUResource.h"
#include "Image.h"

namespace SunEngine
{
	class TextureCube : public GPUResource<BaseTexture>
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