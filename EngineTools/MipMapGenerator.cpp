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
		int baseWidth;
		int baseHeight;

		int mipWidth;
		int mipHeight;

		int kernelSize;
		int mipLevel;

		Pixel* basePixels;
		Pixel** mipPixels;
	};

	void GenMips(const Vector<GenMipMapData>& mipWork)
	{
		for (uint w = 0; w < mipWork.size(); w++)
		{
			const GenMipMapData& data = mipWork.at(w);
			Pixel* mipPixels = new Pixel[data.mipWidth * data.mipHeight];

			float remapX = (float)data.baseWidth / (float)data.mipWidth;
			float remapY = (float)data.baseHeight / (float)data.mipHeight;

			int k = data.kernelSize;

			for (int y = 0; y < data.mipHeight; y++)
			{
				int yIdx = (int)((float)y * remapY);
				for (int x = 0; x < data.mipWidth; x++)
				{
					int xIdx = (int)((float)x * remapX);

					float avgColor[4] = {};
					float samples = 0.0f;

					for (int i = -k; i <= k; i++)
					{
						int iy = yIdx + i;
						if (iy > -1 && iy < data.baseHeight)
						{
							for (int j = -k; j <= k; j++)
							{
								int jx = j + xIdx;
								if (jx > -1 && jx < data.baseWidth)
								{
									Pixel pixel = data.basePixels[(iy * data.baseWidth + jx)];
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
					if (data.mipLevel < (int)MipLayerColors.size())
					{
						float t = 0.5f;
						float alpha = avgColor.w;
						avgColor = avgColor * (1.0f - t) + MipLayerColors[data.mipLevel] * t;
						avgColor.w = alpha;
					}
#endif

					int pixelIndex = (y * data.mipWidth + x);
					mipPixels[pixelIndex].R = (uchar)fmax(fmin(avgColor[0], 255.0f), 0.0f);
					mipPixels[pixelIndex].G = (uchar)fmax(fmin(avgColor[1], 255.0f), 0.0f);
					mipPixels[pixelIndex].B = (uchar)fmax(fmin(avgColor[2], 255.0f), 0.0f);
					mipPixels[pixelIndex].A = (uchar)fmax(fmin(avgColor[3], 255.0f), 0.0f);

				}
			}
			*data.mipPixels = mipPixels;
		}
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

			_mipMaps.resize(numMips);

			width = baseImage.Width;
			height = baseImage.Height;

			const uint NUM_THREADS = 8;
			Vector<GenMipMapData> threadData[NUM_THREADS];

			float minKernel = 1;
			float maxKernel = 4;

			for (int i = 0; i < numMips; i++)
			{
				int mipLevel = i;

				width /= 2;
				height /= 2;

				height = height < MIN_MIP_SIZE ? MIN_MIP_SIZE : height;
				width = width < MIN_MIP_SIZE ? MIN_MIP_SIZE : width;

				_mipMaps[mipLevel].Width = width;
				_mipMaps[mipLevel].Height = height;

				GenMipMapData data;
				data.baseWidth = baseImage.Width;
				data.baseHeight = baseImage.Height;
				data.basePixels = baseImage.Pixels;
				data.mipWidth = width;
				data.mipHeight = height;
				data.kernelSize = (int)(minKernel + (((float)(i+1) / numMips) * (maxKernel - minKernel)));
				data.mipLevel = mipLevel;
				data.mipPixels = &_mipMaps[mipLevel].Pixels;

				if (i < NUM_THREADS)
					threadData[i].push_back(data);
				else
					threadData[rand() % NUM_THREADS].push_back(data);
			}

			Vector<std::thread> threads;
			for (uint i = 0; i < NUM_THREADS; i++)
			{
				threads.push_back(std::thread(GenMips, threadData[i]));
			}

			for (uint i = 0; i < NUM_THREADS; i++)
			{
				threads[i].join();
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