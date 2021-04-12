#pragma once

namespace SunEngine
{
	enum FilterMode
	{
		SE_FM_NEAREST,
		SE_FM_LINEAR,
	};

	enum WrapMode
	{
		SE_WM_CLAMP_TO_EDGE,
		SE_WM_CLAMP_TO_BORDER,
		SE_WM_REPEAT,
	};

	enum AnisotropicMode
	{
		SE_AM_OFF,
		SE_AM_2,
		SE_AM_4,
		SE_AM_8,
		SE_AM_16,
	};

	enum BorderColor
	{
		SE_BC_BLACK,
		SE_BC_WHITE,
	};

	struct SamplerSettings
	{
		FilterMode filterMode;
		WrapMode wrapMode;
		AnisotropicMode anisotropicMode;
		BorderColor borderColor;
	};

}