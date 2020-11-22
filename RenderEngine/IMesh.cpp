#include "VulkanMesh.h"
#include "DirectXMesh.h"

#include "IMesh.h"

namespace SunEngine
{

	IMesh::IMesh()
	{
	}


	IMesh::~IMesh()
	{
	}

	IMesh * IMesh::Allocate(GraphicsAPI api)
	{
		switch (api)
		{
		case SunEngine::SE_GFX_DIRECTX:
			return new DirectXMesh();
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanMesh();
		default:
			return 0;
		}
	}

}