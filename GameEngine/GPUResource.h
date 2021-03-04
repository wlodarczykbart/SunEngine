#pragma once

#include "Resource.h"

namespace SunEngine
{
	template<class T>
	class GPUResource : public Resource
	{
	public:
		GPUResource()
		{

		}

		virtual ~GPUResource()
		{

		}

		virtual bool RegisterToGPU() = 0;

		T* GetGPUObject() { return &_gpuObject; }

	protected:
		T _gpuObject;
	};
}