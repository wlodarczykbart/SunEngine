#pragma once

#include "BaseTexture.h"
#include "GPUResource.h"
#include "Image.h"
#include "glm/glm.hpp"

namespace SunEngine
{
	class Texture2D : public GPUResource<BaseTexture>
	{
	public:
		Texture2D();
		~Texture2D();

		bool RegisterToGPU() override;

		bool Alloc(uint width, uint height);
		bool LoadFromFile(const String& filename);
		bool GenerateMips();

		void FillColor(const glm::vec4& color);
		void Invert();
		void SetPixel(uint x, uint y, const glm::vec4& color);

		uint GetWidth() const { return _img.Width(); }
		uint GetHeight() const { return _img.Height(); }

		void SetSRGB(bool srgb) { _srgb = srgb; }

		const String& GetFilename() const { return _filename; }

	private:
		String _filename;
		Image _img;
		Vector<UniquePtr<Image>> _mips;
		bool _srgb;
	};
}