#pragma once

#include "D3D11Object.h"

namespace SunEngine
{

	class D3D11ShaderResource : public D3D11Object
	{
	public:
		D3D11ShaderResource();
		virtual ~D3D11ShaderResource();

		virtual void BindToShader(class D3D11CommandBuffer* cmdBuffer, class D3D11Shader* pShader, const String& name, uint binding, IBindState* pBindState) = 0;
	};

}