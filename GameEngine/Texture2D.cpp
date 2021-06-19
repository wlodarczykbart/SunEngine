#include "MipMapGenerator.h"
#include "FileBase.h"
#include "Texture2D.h"

namespace SunEngine
{
	Texture2D::Texture2D()
	{
	}

	Texture2D::~Texture2D()
	{
	}

	bool Texture2D::RegisterToGPU()
	{
		if (!_gpuObject.Destroy())
			return false;

		BaseTexture::CreateInfo::TextureData texData = {};
		texData.image = _img.ImageData();

		Vector<ImageData> mipData;
		for (uint i = 0; i < _mips.size(); i++)
		{
			ImageData mipImage = _mips[i]->ImageData();
			mipImage.Flags |= _img.GetFlags();

			mipData.push_back(mipImage);
		}
		texData.mipLevels = mipData.size();
		texData.pMips = mipData.data();

		BaseTexture::CreateInfo info;
		info.numImages = 1;
		info.pImages = &texData;
		if (!_gpuObject.Create(info))
			return false;

		return true;
	}

	bool Texture2D::Alloc(uint width, uint height)
	{
		if (!_img.Allocate(width, height))
			return false;

		_filename.clear();
		return true;
	}

	bool Texture2D::LoadFromFile()
	{
		if (_filename.empty())
			return false;

		if (!_img.Load(_filename))
			return false;

		return true;
	}

	bool Texture2D::LoadFromRAW()
	{
		return LoadRAWInternal(1);
	}

	bool Texture2D::LoadFromRAW16()
	{
		return LoadRAWInternal(2);
	}

	bool Texture2D::LoadFromRAWF32()
	{
		return LoadRAWInternal(4);
	}

	bool Texture2D::LoadRAWInternal(uint byteDivider)
	{
		FileStream file;
		if (!file.OpenForRead(_filename.c_str()))
			return false;
		
		MemBuffer buffer;
		if(!file.ReadBuffer(buffer))
			return false;
		file.Close();

		uint resolutionSquared = buffer.GetSize() / byteDivider;
		uint resolution = uint(sqrtf((float)resolutionSquared));

		if (!Alloc(resolution, resolution))
			return false;

		uchar* p_uchar = (uchar*)buffer.GetData();
		ushort* p_ushort = (ushort*)buffer.GetData();
		float* p_float = (float*)buffer.GetData();

		for (uint y = 0; y < resolution; y++)
		{
			for (uint x = 0; x < resolution; x++)
			{
				uint index = y * resolution + x;
				float value = 0.0f;
				if (byteDivider == 0) value = (float)p_uchar[index];
				else if (byteDivider == 2) value = (float)p_ushort[index];
				else if (byteDivider == 4) value = (float)p_float[index];
				SetPixel(x, y, reinterpret_cast<Pixel&>(value));
			}
		}

		SetSingleFloatTexture();
		return true;
	}

	bool Texture2D::GenerateMips(bool threaded)
	{
		if (_mips.size())
			_mips.clear();

		if (_img.IsCompressed())
			return true;

		MipMapGenerator mipGen;
		if (!mipGen.Create(_img.ImageData(), threaded))
			return false;

		//Pixel Test[] =
		//{
		//	Pixel(1.0f, 0.0f, 0.0f, 1.0f),
		//	Pixel(0.0f, 1.0f, 0.0f, 1.0f),
		//	Pixel(0.0f, 0.0f, 1.0f, 1.0f),
		//	Pixel(1.0f, 1.0f, 0.0f, 1.0f),
		//	Pixel(1.0f, 0.0f, 1.0f, 1.0f),
		//	Pixel(0.0f, 1.0f, 1.0f, 1.0f),
		//	Pixel(1.0f, 1.0f, 1.0f, 1.0f),
		//	Pixel(0.0f, 0.0f, 0.0f, 1.0f),
		//};

		for (uint i = 0; i < mipGen.GetMipLevels(); i++)
		{
			Image* img = new Image();
			if (!img->TransferFrom(mipGen.GetMipMaps()[i]))
				return false;

			//Pixel* p =  img->Pixels();
			//for (uint j = 0; j < img->Width() * img->Height(); j++)
			//{
			//	p[j] = Test[i];
			//}

			_mips.push_back(UniquePtr<Image>(img));
		}

		return  true;
	}

	bool Texture2D::Resize(uint width, uint height)
	{
		if (_mips.size())
			return true;

		if (_img.IsCompressed())
			return true;

		return _img.Resize(width, height);
	}

	bool Texture2D::Compress()
	{
		if (_img.IsCompressed())
			return true;

		if (!_img.Compress())
			return false;

		for (uint i = 0; i < _mips.size(); i++)
		{
			if (!_mips[i]->Compress())
				return false;
		}

		return true;
	}

	void Texture2D::FillColor(const glm::vec4& color)
	{
		Pixel p(color.r, color.g, color.b, color.a);

		for (uint i = 0; i < _img.Width(); i++)
		{
			for (uint j = 0; j < _img.Height(); j++)
			{
				_img.SetPixel(i, j, p);
			}
		}
	}

	void Texture2D::Invert()
	{
		for (uint i = 0; i < _img.Width(); i++)
		{
			for (uint j = 0; j < _img.Height(); j++)
			{
				Pixel p = _img.GetPixel(i, j);
				p.Invert();
				_img.SetPixel(i, j, p);
			}
		}
	}

	void Texture2D::SetPixel(uint x, uint y, const glm::vec4& color)
	{
		_img.SetPixel(x, y, Pixel(color.r, color.g, color.b, color.a));
	}

	void Texture2D::SetPixel(uint x, uint y, const Pixel& color)
	{
		return _img.SetPixel(x, y, color);
	}

	void Texture2D::GetPixel(uint x, uint y, glm::vec4& color) const
	{
		Pixel p = _img.GetPixel(x, y);
		p.Get(color.x, color.y, color.z, color.w);
	}

	void Texture2D::GetPixel(uint x, uint y, Pixel& color) const
	{
		color = _img.GetPixel(x, y);
	}

	void Texture2D::GetFloat(uint x, uint y, float& value) const
	{
		Pixel p = _img.GetPixel(x, y);
		value = *reinterpret_cast<float*>(&p.R);
	}

	void Texture2D::GetAveragePixel(uint x, uint y, int kernelSize, glm::vec4& color) const
	{
		int samples = 0;
		color = Vec4::Zero;

		glm::vec4 tmp;
		for (int i = -kernelSize; i <= kernelSize; i++)
		{
			int yi = (int)y + i;
			if (yi >= 0 && yi < (int)GetHeight())
			{
				for (int j = -kernelSize; j <= kernelSize; j++)
				{
					int xj = (int)x + j;
					if (xj >= 0 && xj < (int)GetWidth())
					{
						GetPixel(xj, yi, tmp);
						color += tmp;
						++samples;
					}
				}
			}
		}

		color /= (float)samples;
	}
}