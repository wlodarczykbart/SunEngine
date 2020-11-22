#include <thread>
#include "MipMapGenerator.h"

namespace SunEngine
{
	//using namespace RMath;

	//Vector<Vec4> MipLayerColors =
	//{
	//	Vec4(255, 0, 0, 255),
	//	Vec4(0, 255, 0, 255),
	//	Vec4(0, 0, 255, 255),
	//	Vec4(255, 255, 0, 255),
	//	Vec4(255, 0, 255, 255),
	//	Vec4(0, 255, 255, 255),
	//	Vec4(255, 255, 255, 255),
	//	Vec4(0, 0, 0, 255),
	//};

	struct GenMipMapData
	{
		int _baseWidth;
		int _baseHeight;

		int _mipWidth;
		int _mipHeight;

		int _kernelSize;
		int _mipLevel;

		Pixel* _basePixels;
		Pixel** _pMipPixels;

		uchar* _complete;

	};

	void GenMips(GenMipMapData* pData)
	{
		Pixel* mipPixels = new Pixel[pData->_mipWidth * pData->_mipHeight];

		float remapX = (float)pData->_baseWidth / (float)pData->_mipWidth;
		float remapY = (float)pData->_baseHeight / (float)pData->_mipHeight;

		int k = pData->_kernelSize;

		for (int y = 0; y < pData->_mipHeight; y++)
		{
			int yIdx = (int)((float)y * remapY);
			for (int x = 0; x < pData->_mipWidth; x++)
			{
				int xIdx = (int)((float)x * remapX);

				float avgColor[4] = {};
				float samples = 0.0f;

				for (int i = -k; i <= k; i++)
				{
					int iy = yIdx + i;
					if (iy > -1 && iy < pData->_baseHeight)
					{
						for (int j = -k; j <= k; j++)
						{
							int jx = j + xIdx;
							if (jx > -1 && jx < pData->_baseWidth)
							{
								Pixel pixel = pData->_basePixels[(iy * pData->_baseWidth + jx)];
								avgColor[0] += pixel.R;
								avgColor[1] += pixel.G;
								avgColor[2] += pixel.B;
								avgColor[3] += pixel.A;
								samples++;
							}
						}
					}
				}

				avgColor[0] /= samples;
				avgColor[1] /= samples;
				avgColor[2] /= samples;
				avgColor[3] /= samples;

#if 0
				if (pData->_mipLevel < (int)MipLayerColors.size())
				{
					float t = 0.5f;
					float alpha = avgColor.w;
					avgColor = avgColor * (1.0f - t) + MipLayerColors[pData->_mipLevel] * t;
					avgColor.w = alpha;
				}
#endif

				int pixelIndex = (y * pData->_mipWidth + x);
				mipPixels[pixelIndex].R = (uchar)fmax(fmin(avgColor[0], 255.0f), 0.0f);
				mipPixels[pixelIndex].G = (uchar)fmax(fmin(avgColor[1], 255.0f), 0.0f);
				mipPixels[pixelIndex].B = (uchar)fmax(fmin(avgColor[2], 255.0f), 0.0f);
				mipPixels[pixelIndex].A = (uchar)fmax(fmin(avgColor[3], 255.0f), 0.0f);

			}
		}

		*pData->_pMipPixels = mipPixels;
		*pData->_complete = 0x1;
	}

	MipMapGenerator::MipMapGenerator()
	{
	}


	MipMapGenerator::~MipMapGenerator()
	{
		for (uint i = 0; i < _mipMaps.size(); i++)
		{
			delete[] _mipMaps[i].Pixels;
		}

		_mipMaps.clear();
	}

	bool MipMapGenerator::Create(const ImageData &baseImage)
	{
		int width = baseImage.Width;
		int height = baseImage.Height;

		if (width > MIN_MIP_SIZE && height > MIN_MIP_SIZE)
		{
			int maxSize = width > height ? width : height;
			int numMips = 0;
			do
			{
				maxSize /= 2;
				numMips++;
			} while (maxSize > MIN_MIP_SIZE);

			SunEngine::Vector<GenMipMapData> mipData;
			mipData.resize(numMips);

			SunEngine::Vector<SunEngine::uchar> mipState;
			mipState.resize(numMips);
			memset(mipState.data(), 0, sizeof(SunEngine::uchar) * numMips);

			SunEngine::Vector<SunEngine::uchar> mipComplete;
			mipComplete.resize(numMips);
			memset(mipComplete.data(), 1, sizeof(SunEngine::uchar) * numMips);

			_mipMaps.resize(numMips);

			width = baseImage.Width;
			height = baseImage.Height;

			int mipLevel = 0;
			int kernel = 1;
			do
			{
				width /= 2;
				height /= 2;

				height = height < MIN_MIP_SIZE ? MIN_MIP_SIZE : height;
				width = width < MIN_MIP_SIZE ? MIN_MIP_SIZE : width;

				_mipMaps[mipLevel].Width = width;
				_mipMaps[mipLevel].Height = height;

				GenMipMapData* pData = &mipData[mipLevel];
				pData->_baseWidth = baseImage.Width;
				pData->_baseHeight = baseImage.Height;
				pData->_basePixels = baseImage.Pixels;
				pData->_mipWidth = width;
				pData->_mipHeight = height;
				pData->_kernelSize = kernel;
				pData->_mipLevel = mipLevel;
				pData->_pMipPixels = &_mipMaps[mipLevel].Pixels;
				pData->_complete = &mipState[mipLevel];

				std::thread thread = std::thread(GenMips, pData);
				thread.detach();
				mipLevel++;
				kernel++;

			} while (width > MIN_MIP_SIZE || height > MIN_MIP_SIZE);

			int counter = 0;
			while (memcmp(mipState.data(), mipComplete.data(), sizeof(uchar) * numMips) != 0)
			{
				for (int c = 0; c < 32; c++)
				{
					counter++;
				}
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	uint MipMapGenerator::GetMipLevels() const
	{
		return (uint)_mipMaps.size();
	}

	ImageData* MipMapGenerator::GetMipMaps() const
	{
		return (ImageData*)_mipMaps.data();
	}

}