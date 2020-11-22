#include "GraphicsContext.h"
#include "DirectXObject.h"

namespace SunEngine
{

	DirectXObject::DirectXObject()
	{
		_device = static_cast<DirectXDevice*>(GraphicsContext::GetDevice());
	}


	DirectXObject::~DirectXObject()
	{
	}

}