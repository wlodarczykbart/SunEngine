#include "ResourceMgr.h"
#include "Texture2DArray.h"

namespace SunEngine
{
	Texture2DArray::Texture2DArray()
	{
		_width = 0;
		_height = 0;
	}

	Texture2DArray::~Texture2DArray()
	{

	}

	bool Texture2DArray::RegisterToGPU()
	{
		if (_width == 0 || _height == 0 || _textures.size() == 0)
			return false;

		Vector<BaseTexture::CreateInfo::TextureData> imageInfos;
		imageInfos.resize(_textures.size());

		Vector<Vector<ImageData>> mipDataLookup;
		mipDataLookup.resize(_textures.size());

		for (uint i = 0; i < _textures.size(); i++)
		{
			auto tex = _textures[i].get();
			
			mipDataLookup[i].resize(tex->texture.GetMipCount());
			for (uint j = 0; j < mipDataLookup[i].size(); j++)
				mipDataLookup[i][j] = tex->texture.GetMipImageData(j);

			BaseTexture::CreateInfo::TextureData texInfo = {};
			texInfo.image = tex->texture.GetImageData();
			texInfo.mipLevels = mipDataLookup[i].size();
			texInfo.pMips = mipDataLookup[i].data();

			imageInfos[i] = texInfo;
		}

		BaseTexture::CreateInfo info = {};
		info.numImages = _textures.size();
		info.pImages = imageInfos.data();
		
		if (!_gpuObject.Create(info))
			return false;

		return true;
	}

	bool Texture2DArray::AddTexture(Texture2D* pTexture)
	{
		if (_width == 0 || _height == 0)
			return false;

		//TODO: return on compressed texutre?

		TextureEntry* pEntry = new TextureEntry();
		if (!pEntry->texture.Alloc(_width, _height))
			return false;

		_textures.push_back(UniquePtr<TextureEntry>(pEntry));
		if (!SetTexture(_textures.size() - 1, pTexture))
			return false;

		return true;
	}

	bool Texture2DArray::SetTexture(uint index, Texture2D* pTexture)
	{
		if (index < _textures.size())
		{
			TextureEntry* pEntry = _textures[index].get();
			pEntry->pSrc = pTexture;

			if (!ResourceMgr::Get().IsDefaultTexture2D(pTexture))
			{
				float mapx = (float)pTexture->GetWidth() / _width;
				float mapy = (float)pTexture->GetHeight() / _height;

				for (uint y = 0; y < _height; y++)
				{
					for (uint x = 0; x < _width; x++)
					{
						glm::vec4 srcColor;
						pTexture->GetPixel(uint(x * mapx), uint(y * mapy), srcColor);
						pEntry->texture.SetPixel(x, y, srcColor);
					}
				}
			}
			else
			{
				glm::vec4 srcColor;
				pTexture->GetPixel(0, 0, srcColor);
				pEntry->texture.FillColor(srcColor);
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	bool Texture2DArray::GenerateMips(bool threaded)
	{
		//handle threading with ThreadPool...

		for (auto& tex : _textures)
		{
			if (!tex->texture.GenerateMips(false))
				return false;
		}

		return true;
	}

	bool Texture2DArray::Compress()
	{
		for (auto& tex : _textures)
		{
			if (!tex->texture.Compress())
				return false;
		}

		return true;
	}

	void Texture2DArray::SetSRGB()
	{
		for (auto& tex : _textures)
			tex->texture.SetSRGB();
	}

	void Texture2DArray::SetPixel(uint x, uint y, uint i, const Pixel& color)
	{
		_textures[i]->texture.SetPixel(x, y, color);
	}

	void Texture2DArray::SetPixel(uint x, uint y, uint i, const glm::vec4& color)
	{
		_textures[i]->texture.SetPixel(x, y, color);
	}

}