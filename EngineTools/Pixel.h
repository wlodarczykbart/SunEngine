#pragma once

#include "Types.h"

namespace SunEngine
{

	struct Pixel
	{
		Pixel() { R = G = B = A = 0; }

		Pixel(float r, float g, float b, float a)
		{
			Set(r, g, b, a);
		}

		uchar R;
		uchar G;
		uchar B;
		uchar A;

		inline void SwapRB()
		{
			uchar tmp = R; 
			R = B;
			B = tmp;
		}

		inline void Invert()
		{
			R = 255 - R;
			G = 255 - G;
			B = 255 - B;
			A = 255 - A;
		}

		inline uchar Grayscale()  const
		{
			return (uchar)((((float)R + (float)G + (float)B) / 3.0f));
		}

		void Set(float r, float g, float b, float a)
		{
			R = uchar(std::clamp(r, 0.0f, 1.0f) * 255.0f);
			G = uchar(std::clamp(g, 0.0f, 1.0f) * 255.0f);
			B = uchar(std::clamp(b, 0.0f, 1.0f) * 255.0f);
			A = uchar(std::clamp(a, 0.0f, 1.0f) * 255.0f);
		}

		void Get(float& r, float& g, float& b, float& a)
		{
			r = R / 255.0f;
			g = G / 255.0f;
			b = B / 255.0f;
			a = A / 255.0f;
		}
	};

}