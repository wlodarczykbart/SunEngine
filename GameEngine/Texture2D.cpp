#include "MipMapGenerator.h"
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

		BaseTexture::CreateInfo info = {};
		info.image = _img.ImageData();

		Vector<ImageData> mipData;
		for (uint i = 0; i < _mips.size(); i++)
		{
			ImageData mipImage = _mips[i]->ImageData();
			mipImage.Flags |= _img.GetFlags();

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

	bool Texture2D::LoadFromFile()
	{
		if (_filename.empty())
			return false;

		if (!_img.Load(_filename))
			return false;

		return true;
	}

	bool Texture2D::GenerateMips(bool threaded)
	{
		if (_mips.size())
			return true;

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
}