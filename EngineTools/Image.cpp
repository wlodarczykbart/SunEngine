#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"

#include "FileBase.h"

#include "Image.h"

namespace SunEngine
{
	typedef uint(*QueryCompressedSize)(uint width, uint height);

	Map<uint, QueryCompressedSize> FormatSizeFuncs
	{
		{ ImageData::NONE, [](uint width, uint height) -> uint { return width * height; } },
		{ ImageData::COMPRESSED_BC1, [](uint width, uint height) -> uint { return (width * height / 16) * 2; } },
		{ ImageData::COMPRESSED_BC3, [](uint width, uint height) -> uint { return (width * height / 16) * 4; } },
		{ ImageData::SAMPLED_TEXTURE_R32G32B32A32F, [](uint width, uint height) -> uint { return (width * height) * 4; } },
	};

	struct TGAHeader
	{
		 uchar idLength;
		 uchar colorMapType;
		 uchar imageType;
		 ushort colorMapIndex;
		 ushort colorMapLength;
		 uchar colorMapSize;
		 ushort xOrigin;
		 ushort yOrigin;
		 ushort width;
		 ushort height;
		 uchar bitsPerPixel;
		 uchar imageDescriptor;
	};

	Image::Image()
	{
		_width = 0;
		_height = 0;
		_pixels = 0;
		_internalFlags = 0;
	}


	Image::~Image()
	{
		CleanUp();
	}

	bool Image::Load(const String& filename)
	{
		int width, height, comp;
		stbi_uc* pixels = stbi_load(filename.data(), &width, &height, &comp, STBI_rgb_alpha);
		if (pixels)
		{
			CleanUp();
			_pixels = (Pixel*)pixels;
			_width = (uint)width;
			_height = (uint)height;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Image::Allocate(uint width, uint height, const Pixel* pixels, uint flags)
	{
		if (width == 0 || height == 0)
			return false;

		uint bufferSize = FormatSizeFuncs.at(GetImageFormat(flags))(width, height) * sizeof(Pixel);

		CleanUp();
		_pixels = (Pixel*)STBI_MALLOC(bufferSize);
		_width = width;
		_height = height;
		_internalFlags |= flags;

		if (pixels)
			memcpy(_pixels, pixels, bufferSize);

		return true;
	}

	bool Image::CreateFrom(const Image * pOther, uint width, uint height)
	{
		if (pOther == 0)
			return false;

		if (pOther->_pixels == 0 || pOther->_width == 0 || pOther->_height == 0)
			return false;

		return CreateFrom(pOther->ImageData(), width, height);
	}

	bool Image::CreateFrom(const SunEngine::ImageData& data, uint width, uint height)
	{
		bool resizing = false;
		if (data.Pixels == _pixels)
		{
			//null out member so we don't delete it
			resizing = true;
			_pixels = 0;
		}

		if (!Allocate(width, height, 0, data.Flags))
			return false;

		float mapX = (float)(data.Width - 1) / (_width - 1);
		float mapY = (float)(data.Height - 1) / (_height - 1);

		if (mapX == 1.0f && mapY == 1.0f)
		{
			uint bufferSize = FormatSizeFuncs.at(GetImageFormat(data.Flags))(width, height) * sizeof(Pixel);
			memcpy(_pixels, data.Pixels, bufferSize);
		}
		else
		{
			//TODO...how to resize compressed? not supporting right now
			if (GetImageFormat(data.Flags))
			{
				if (resizing)
					STBI_FREE(data.Pixels);

				return false;
			}

			for (uint y = 0; y < height; y++)
			{
				uint otherY = (uint)((float)y * mapY);
				for (uint x = 0; x < width; x++)
				{
					uint otherX = (uint)((float)x * mapX);
					_pixels[y * _width + x] = data.Pixels[otherY * data.Width + otherX];
				}
			}
		}

		//we own these...
		if (resizing)
			STBI_FREE(data.Pixels);

		return true;
	}

	bool Image::Resize(uint width, uint height)
	{
		return CreateFrom(ImageData(), width, height);
	}

	bool Image::TransferFrom(SunEngine::ImageData& data)
	{
		CleanUp();

		_pixels = data.Pixels;
		_width= data.Width;
		_height = data.Height;
		_internalFlags = data.Flags;

		memset(&data, 0x0, sizeof(data));
		return true;
	}

	void Image::SwapRB()
	{
		if (_pixels)
		{
			for (uint i = 0; i < _width * _height; i++)
			{
				uchar tmp = _pixels[i].R;
				_pixels[i].R = _pixels[i].B;
				_pixels[i].B = tmp;
			}
		}
	}
		 
	uint Image::Width() const
	{
		return _width;
	}

	uint Image::Height() const
	{
		return _height;
	}

	Pixel* Image::Pixels() const
	{
		return _pixels;
	}

	Pixel Image::GetPixel(uint x, uint y) const
	{
		if (x < _width && y < _height)
		{
			return _pixels[y * _width + x];
		}
		else
		{
			return Pixel();
		}
	}

	Pixel Image::GetPixelClamped(int x, int y) const
	{
		x = x < 0 ? 0 : x >= (int)_width ? (int)_width - 1 : x;
		y = y < 0 ? 0 : y >=(int) _height ? (int)_height - 1 : y;

		return _pixels[y * _width + x];
	}

	void Image::SetPixel(uint x, uint y, const Pixel& p)
	{
		if (x < _width && y < _height)
		{
			_pixels[y * _width + x] = p;
		}
	}

	bool Image::SaveAsTGA(const String& filename)
	{
		FileStream fw;
		if (!fw.OpenForWrite(filename.data()))
			return false;

		TGAHeader hdr = {};

		hdr.idLength = 0;
		hdr.colorMapType = 0;
		hdr.imageType = 2;
		hdr.colorMapIndex = 0;
		hdr.colorMapLength = 0;
		hdr.colorMapSize = 0;
		hdr.xOrigin = 0;
		hdr.yOrigin = 0;
		hdr.width = (ushort)_width;
		hdr.height = (ushort)_height;
		hdr.bitsPerPixel = 32;
		hdr.imageDescriptor = 0;

		if(!fw.Write(hdr.idLength)) return false;
		if(!fw.Write(hdr.colorMapType)) return false;
		if(!fw.Write(hdr.imageType)) return false;
		if(!fw.Write(hdr.colorMapIndex)) return false;
		if(!fw.Write(hdr.colorMapLength)) return false;
		if(!fw.Write(hdr.colorMapSize)) return false;
		if(!fw.Write(hdr.xOrigin)) return false;
		if(!fw.Write(hdr.yOrigin)) return false;
		if(!fw.Write(hdr.width)) return false;
		if(!fw.Write(hdr.height)) return false;
		if(!fw.Write(hdr.bitsPerPixel)) return false;
		if(!fw.Write(hdr.imageDescriptor)) return false;

		Vector<Pixel> filePixels;
		filePixels.resize(_width * _height);
		for (uint y = 0; y < _height; y++)
		{
			for (uint x = 0; x < _width; x++)
			{
				Pixel p = GetPixel(x, _height - y - 1);
				p.SwapRB();
				filePixels[y * _width + x] = p;
			}
		}
		fw.Write(filePixels.data(), sizeof(Pixel) * filePixels.size());

		fw.Close();
		return true;
	}

	void Image::CleanUp()
	{
		if (_pixels)
		{
			STBI_FREE(_pixels);
			_pixels = 0;
		}

		_width = 0;
		_height = 0;
	}


	ImageData Image::ImageData() const
	{
		SunEngine::ImageData id;
		id.Width = _width;
		id.Height = _height;
		id.Pixels = _pixels;
		id.Flags |= _internalFlags;
		return id;
	}

	bool Image::Write(StreamBase& stream)
	{
		if (!stream.Write(_width))
			return false;

		if (!stream.Write(_height))
			return false;

		if (!stream.Write(_internalFlags))
			return false;

		if (_pixels)
		{
			uint bufferSize = FormatSizeFuncs.at(GetImageFormat(_internalFlags))(_width, _height) * sizeof(Pixel);
			if (!stream.Write(_pixels, bufferSize))
				return false;
		}

		return true;
	}

	bool Image::Read(StreamBase& stream)
	{
		if (!stream.Read(_width))
			return false;

		if (!stream.Read(_height))
			return false;

		if (!stream.Read(_internalFlags))
			return false;

		Allocate(_width, _height, 0, _internalFlags);
		if (_pixels)
		{
			uint bufferSize = FormatSizeFuncs.at(GetImageFormat(_internalFlags))(_width, _height) * sizeof(Pixel);
			if (!stream.Read(_pixels, bufferSize))
				return false;
		}

		return true;
	}

	bool Image::Compress()
	{
		uint widthPadded = 4 * ((_width + 3) / 4);
		uint heightPadded = 4 * ((_height + 3) / 4);

		uchar alpha = _pixels[0].A;
		int compressAlpha = 0;
		for (uint i = 1; i < _width * _height; i++)
		{
			if (alpha != _pixels[i].A)
			{
				compressAlpha = 1;
				break;
			}
		}

		//compressAlpha = 1 - compressAlpha;

		if (compressAlpha == 0)
			_internalFlags |= ImageData::COMPRESSED_BC1;
		else
			_internalFlags |= ImageData::COMPRESSED_BC3;

		Pixel srcBlock[16];
		Pixel dstBlock[4]; //2 = no alpha, 4 = alpha

		uint compressedPixelCount = FormatSizeFuncs.at(GetImageFormat(_internalFlags))(widthPadded, heightPadded);
		Pixel* dstPixels = new Pixel[compressedPixelCount];
		//dstPixels.resize(compressedSize);
		uint compressedCounter = 0;

		uint pixelsPerBlock = FormatSizeFuncs.at(GetImageFormat(_internalFlags))(4, 4);

		for (uint y = 0; y < heightPadded; y += 4)
		{
			for (uint x = 0; x < widthPadded; x += 4)
			{
				for (uint i = 0; i < 4; i++)
				{
					uint iy = i + y;
					if (iy >= _height) iy = _height - 1;
					for (uint j = 0; j < 4; j++)
					{
						uint jx = j + x;
						if (jx >= _width) jx = _width - 1;
							
						srcBlock[i * 4 + j] = GetPixel(jx, iy);
					}
				}

				stb_compress_dxt_block(&dstBlock[0].R, &srcBlock[0].R, compressAlpha, STB_DXT_HIGHQUAL);

				for (uint i = 0; i < pixelsPerBlock; i++)
				{
					dstPixels[compressedCounter++] = dstBlock[i];
				}
			}
		}
		
		delete[] _pixels;
		_pixels = dstPixels;
		_width = widthPadded;
		_height = heightPadded;

		return true;
	}

	bool ReadBufferAs16BitGrayscaleImage(const void* pData, uint size, int& width, int& height, Vector<unsigned short>& output)
	{
		int nChannels = 0;
		ushort* pShortBuff = stbi_load_16_from_memory((const uchar*)pData, size, &width, &height, &nChannels, STBI_grey);
		if (!pShortBuff)
			return false;

		output.resize(width * height);
		memcpy(output.data(), pShortBuff, width * height * sizeof(ushort));
		return true;
	}

	uint GetImageFormat(uint flags)
	{
		if (flags & ImageData::COMPRESSED_BC1)
			return ImageData::COMPRESSED_BC1;

		if (flags & ImageData::COMPRESSED_BC3)
			return ImageData::COMPRESSED_BC3;
	
		if (flags & ImageData::SAMPLED_TEXTURE_R32G32B32A32F)
			return ImageData::SAMPLED_TEXTURE_R32G32B32A32F;

		return 0;
	}

	namespace
	{
		uint as_uint(const float x) {
			return *(uint*)&x;
		}
		float as_float(const uint x) {
			return *(float*)&x;
		}
	}

	float HalfToFloat(ushort x)
	{
		const uint e = (x & 0x7C00) >> 10; // exponent
		const uint m = (x & 0x03FF) << 13; // mantissa
		const uint v = as_uint((float)m) >> 23; // evil log2 bit hack to count leading zeros in denormalized format
		return as_float((x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) * ((v - 37) << 23 | ((m << (150 - v)) & 0x007FE000))); // sign : normalized : denormalized
	}

	ushort FloatToHalf(float x)
	{
		const uint b = as_uint(x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
		const uint e = (b & 0x7F800000) >> 23; // exponent
		const uint m = b & 0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding
		return (b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) | ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) | (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
	}

	uint ImageData::GetImageSize() const
	{
		return FormatSizeFuncs.at(GetImageFormat(Flags))(Width, Height) * sizeof(Pixel);
	}

}