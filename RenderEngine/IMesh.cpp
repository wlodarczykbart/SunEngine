#include "VulkanMesh.h"
#include "D3D11Mesh.h"
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
		case SunEngine::SE_GFX_VULKAN:
			return new VulkanMesh();
		case SunEngine::SE_GFX_D3D11:
			return new D3D11Mesh();
		default:
			return 0;
		}
	}

}