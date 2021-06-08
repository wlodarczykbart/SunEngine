#pragma once

#include "BaseTexture.h"
#include "GPUResource.h"
#include "Image.h"

namespace SunEngine
{
	class Texture2D : public GPUResource<BaseTexture>
	{
	public:
		Texture2D();
		~Texture2D();

		bool RegisterToGPU() override;

		bool Alloc(uint width, uint height);
		bool GenerateMips(bool threaded);
		bool Resize(uint width, uint height);
		bool Compress();

		void FillColor(const glm::vec4& color);
		void Invert();
		void SetPixel(uint x, uint y, const glm::vec4& color);
		void SetPixel(uint x, uint y, const Pixel& color);
		void GetPixel(uint x, uint y, glm::vec4& color) const;
		void GetPixel(uint x, uint y, Pixel& color) const;
		void GetFloat(uint x, uint y, float& value) const; //for use in a SAMPLED_TEXTURE_R32F texture
		void GetAveragePixel(uint x, uint y, int kernelSize, glm::vec4& color) const;

		uint GetWidth() const { return _img.Width(); }
		uint GetHeight() const { return _img.Height(); }

		void SetSRGB() { _img.SetFlags(ImageData::SRGB); }
		void SetSingleFloatTexture() { _img.SetFlags(ImageData::SAMPLED_TEXTURE_R32F); }

		uint GetImageFlags() const { return _img.GetFlags(); }

		void SetFilename(const String& filename) { _filename = filename; }
		const String& GetFilename() const { return _filename; }
		bool LoadFromFile();
		bool LoadFromRAW();
		bool LoadFromRAW16();
		bool LoadFromRAWF32();

		ImageData GetImageData() const { return _img.ImageData(); }
		ImageData GetMipImageData(uint index) const { return _mips[index]->ImageData(); }
		uint GetMipCount() const { return _mips.size(); }

	private:
		bool LoadRAWInternal(uint byteDivider);

		String _filename;
		Image _img;
		Vector<UniquePtr<Image>> _mips;
	};
}