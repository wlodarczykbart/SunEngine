#pragma once

#include "DirectXObject.h"

namespace SunEngine
{

	class DirectXShaderResource : public DirectXObject
	{
	public:
		DirectXShaderResource();
		virtual ~DirectXShaderResource();

		virtual void BindToShader(class DirectXShader* pShader, uint binding) = 0;
	};

}