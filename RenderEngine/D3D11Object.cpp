#include "GraphicsContext.h"
#include "D3D11Object.h"

namespace SunEngine
{

	D3D11Object::D3D11Object()
	{
		_device = static_cast<D3D11Device*>(GraphicsContext::GetDevice());
	}


	D3D11Object::~D3D11Object()
	{
	}

}