#pragma once

#include "Types.h"

namespace SunEngine
{

	struct Pixel
	{
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

		inline uchar Grayscale()  const
		{
			return (uchar)((((float)R + (float)G + (float)B) / 3.0f));
		}
	};

}