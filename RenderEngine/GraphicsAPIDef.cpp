#include "ITexture.h"
#include "ISampler.h"
#include "ISurface.h"
#include "IShader.h"
#include "IShaderBindings.h"
#include "IMesh.h"
#include "IDevice.h"
#include "IGraphicsPipeline.h"
#include "IRenderTarget.h"
#include "ICommandBuffer.h"
#include "IUniformBuffer.h"
#include "IVRInterface.h"

#include "GraphicsAPIDef.h"

namespace SunEngine
{
	GraphicsAPI g_API = SE_GFX_VULKAN;

	void SunEngine::SetGraphicsAPI(GraphicsAPI api)
	{
		g_API = api; //Vulkan support not up to date...
		//g_API = SE_GFX_D3D11;
	}

	GraphicsAPI GetGraphicsAPI()
	{
		return g_API;
	}


	template<typename T>
	T * AllocateGraphics()
	{
		return T::Allocate(g_API);
	}

	void RegisterFunc()
	{
		AllocateGraphics<ITexture>();
		AllocateGraphics<ISampler>();
		AllocateGraphics<ISurface>();
		AllocateGraphics<IShader>();
		AllocateGraphics<IShaderBindings>();
		AllocateGraphics<IMesh>();
		AllocateGraphics<IGraphicsPipeline>();
		AllocateGraphics<IDevice>();
		AllocateGraphics<IRenderTarget>();
		AllocateGraphics<ICommandBuffer>();
		AllocateGraphics<IUniformBuffer>();
		AllocateGraphics<IVRInterface>();
	}

}