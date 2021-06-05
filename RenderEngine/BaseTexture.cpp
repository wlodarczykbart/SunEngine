#include "FileReader.h"
#include "MemBuffer.h"
#include "ITexture.h"
#include "BaseTexture.h"

namespace SunEngine
{

	BaseTexture::BaseTexture() : GraphicsObject(GraphicsObject::TEXTURE)
	{
		_iTexture = 0;
		_width = 0;
		_height = 0;
	}


	BaseTexture::~BaseTexture()
	{
	}

	//bool BaseTexture::Create(const char * tgaFilename)
	//{
	//	const uint TGA_HEADER_SIZE = 18;
	//	const uint TGA_IMAGE_TYPE_OFFSET = 2;
	//	const uint TGA_IMAGE_SPEC_OFFSET = 8;

	//	const uint TGA_UNCOMPRESSED_TRUE_COLOR = 2;

	//	uchar hdr[TGA_HEADER_SIZE];

	//	FileReader rdr;
	//	if (rdr.Open(tgaFilename))
	//	{
	//		rdr.Read(hdr, TGA_HEADER_SIZE);

	//		uchar imageType = *(hdr + TGA_IMAGE_TYPE_OFFSET);
	//		if (imageType == TGA_UNCOMPRESSED_TRUE_COLOR)
	//		{
	//			CreateInfo info;

	//			uint TGA_WIDTH_OFFSET = 2;
	//			uint TGA_HEIGHT_OFFSET = 3;
	//			uint TGA_PIXEL_DEPTH_OFFSET = 8;

	//			uchar *imgSpec = hdr + TGA_IMAGE_SPEC_OFFSET;
	//			info.width = *((ushort*)imgSpec + TGA_WIDTH_OFFSET);
	//			info.height = *((ushort*)imgSpec + TGA_HEIGHT_OFFSET);
	//			uint components = *(imgSpec + TGA_PIXEL_DEPTH_OFFSET) / 8;

	//			MemBuffer buffer;
	//			buffer.SetSize(info.width*info.height*components);

	//			rdr.Read(buffer.GetData(), buffer.GetSize());

	//			info.pixels = new Pixel[info.width * info.height];

	//			if (components == 4)
	//			{
	//				memcpy(info.pixels, buffer.GetData(), buffer.GetSize());
	//			}
	//			else
	//			{
	//				uchar* pBuffer = (uchar*)buffer.GetData();
	//				for (uint i = 0; i < info.height * info.width; i++)
	//				{
	//					uchar* rgb = pBuffer + i * components;
	//					info.pixels[i] = { 0, 0, 0, 255 };
	//					uchar* rgba = &info.pixels[i].r;
	//					for (uint j = 0; j < components; j++)
	//					{
	//						rgba[j] = rgb[j];
	//					}

	//				}
	//			}

	//			for (uint i = 0; i < info.width*info.height; i++)
	//			{

	//				uchar tmp = info.pixels[i].b;
	//				info.pixels[i].b = info.pixels[i].r;
	//				info.pixels[i].r = tmp;
	//			}

	//			_pixels = info.pixels;
	//			bool created =  Create(info);
	//			rdr.Close();
	//			return created;
	//		}
	//		else
	//		{
	//			rdr.Close();
	//			return false;
	//		}
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	bool BaseTexture::Create(const CreateInfo & info)
	{
		if (!Destroy())
			return false;

		if (!_iTexture)
			_iTexture = AllocateGraphics<ITexture>();

		if (!info.isExternal)
		{
			ITextureCreateInfo apiInfo = {};
			apiInfo.image = info.image;
			apiInfo.pMips = info.pMips;
			apiInfo.mipLevels = info.mipLevels;

			if (!_iTexture->Create(apiInfo))
			{
				_errStr = "Failed to create API BaseTexture";
				return false;
			}
		}

		_width = info.image.Width;
		_height = info.image.Height;

		return true;
	}

	bool BaseTexture::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_iTexture = 0;
		return true;
	}

	IObject * BaseTexture::GetAPIHandle() const
	{
		return _iTexture;
	}

	uint BaseTexture::GetWidth() const
	{
		return _width;
	}

	uint BaseTexture::GetHeight() const
	{
		return _height;
	}
}