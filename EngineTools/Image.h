#pragma once

#include  "Pixel.h"
#include "Serializable.h"

namespace SunEngine
{
	struct ImageData
	{
		enum Flags
		{
			NONE = 0,
			COLOR_BUFFER_RGBA8 = 1 << 0,
			COLOR_BUFFER_RGBA16F = 1 << 1,
			DEPTH_BUFFER = 1 << 2,
			COMPRESSED_BC1 = 1 << 3,
			COMPRESSED_BC3 = 1 << 4,
			SAMPLED_TEXTURE_R32F = 1 << 5,
			SAMPLED_TEXTURE_R32G32B32A32F = 1 << 6,
			SRGB = 1 << 7,
		};

		ImageData()
		{
			Pixels = 0;
			Width = 0;
			Height = 0;
			Flags = NONE;
		}

		ImageData(uint width, uint height, Pixel* pixels, uint flags = Flags::NONE)
		{
			Width = width;
			Height = height;
			Pixels = pixels;
			Flags = flags;
		}

		uint GetImageSize() const;

		Pixel* Pixels;
		uint Width;
		uint Height;
		uint Flags;
	};

	bool ReadBufferAs16BitGrayscaleImage(const void* pData, uint size, int& width, int& height, Vector<ushort>& output);
	uint GetImageFormat(uint flags);

	float HalfToFloat(ushort x);
	ushort FloatToHalf(float x);

	class Image final : public Serializable
	{
	public:
		Image();
		Image(const Image& rhs) = delete;
		Image & operator = (const Image&) = delete;
		~Image();

		bool Load(const String& filename);
		bool Allocate(uint width, uint height, const Pixel* pixels = 0, uint flags = 0);
		bool CreateFrom(const Image* pOther, uint width, uint height);
		bool CreateFrom(const ImageData& data, uint width, uint height);
		bool Resize(uint width, uint height);
		bool TransferFrom(ImageData& data);

		void SwapRB();

		uint Width() const;
		uint Height() const;
		Pixel* Pixels() const;

		Pixel GetPixel(uint x, uint y) const;
		Pixel GetPixelClamped(int x, int y) const;
		void SetPixel(uint x, uint y, const Pixel& p);

		bool SaveAsTGA(const String& filename);

		ImageData ImageData() const;

		bool Write(StreamWriter& stream) override;
		bool Read(StreamReader& stream) override;

		bool Compress();

		inline uint GetFlags() const { return _internalFlags; }

	private:
		void CleanUp();

		Pixel* _pixels;
		uint _width;
		uint _height;
		uint _internalFlags;
	};
}