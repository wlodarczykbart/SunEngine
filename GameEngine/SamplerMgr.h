#pragma once

#include "Sampler.h"

namespace SunEngine
{
	class SamplerMgr
	{
	public:
		SamplerMgr(const SamplerMgr&) = delete;
		SamplerMgr& operator = (const SamplerMgr&) = delete;

		static SamplerMgr& Get();

	private:
		SamplerMgr();
		~SamplerMgr();
	};
}