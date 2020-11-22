#include "ITexture.h"
#include "ISampler.h"
#include "ISurface.h"
#include "IShader.h"
#include "IShaderBindings.h"
#include "IMesh.h"
#include "IDevice.h"
#include "IGraphicsPipeline.h"
#include "IRenderTarget.h"
#include "ITextureCube.h"
#include "ITextureArray.h"
#include "ICommandBuffer.h"
#include "IUniformBuffer.h"

#include "GraphicsAPIDef.h"

namespace SunEngine
{
	GraphicsAPI g_API = SE_GFX_DIRECTX;

	void SunEngine::SetGraphicsAPI(GraphicsAPI api)
	{
		g_API = api; //Vulkan support not up to date...
		//g_API = SE_GFX_DIRECTX;
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
		AllocateGraphics<ITextureCube>();
		AllocateGraphics<IRenderTarget>();
		AllocateGraphics<ITextureArray>();
		AllocateGraphics<ICommandBuffer>();
		AllocateGraphics<IUniformBuffer>();
	}

}