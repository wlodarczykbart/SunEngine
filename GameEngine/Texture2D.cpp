#include "MipMapGenerator.h"
#include "Texture2D.h"

namespace SunEngine
{
	Texture2D::Texture2D()
	{
		_srgb = false;
	}

	Texture2D::~Texture2D()
	{
	}

	bool Texture2D::RegisterToGPU()
	{
		if (!_gpuObject.Destroy())
			return false;

		BaseTexture::CreateInfo info = {};
		info.image = _img.ImageData();
		if (_srgb)
			info.image.Flags |= ImageData::SRGB;

		Vector<ImageData> mipData;
		for (uint i = 0; i < _mips.size(); i++)
		{
			ImageData mipImage = _mips[i]->ImageData();
			if (_srgb)
				mipImage.Flags |= ImageData::SRGB;

			mipData.push_back(mipImage);
		}
		info.mipLevels = mipData.size();
		info.pMips = mipData.data();

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

	bool Texture2D::LoadFromFile(const String& filename)
	{
		if (!_img.Load(filename))
			return false;

		_filename = filename;
		return true;
	}

	bool Texture2D::GenerateMips()
	{
		if (_mips.size())
			return true;

		MipMapGenerator mipGen;
		if (!mipGen.Create(_img.ImageData()))
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
}