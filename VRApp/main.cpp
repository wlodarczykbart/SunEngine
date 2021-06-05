#include "GraphicsContext.h"
#include "VRInterface.h"
#include "RenderTarget.h"

using namespace SunEngine;

int main(int argc, const char** argv)
{
	SetGraphicsAPI(SE_GFX_D3D11);
	bool debugEnabled = true;

	GraphicsContext context;
	{
		GraphicsContext::CreateInfo info = {};
		info.debugEnabled = debugEnabled;
		info.vrEnabled = true;
		if (!context.Create(info))
			return -1;
	}

	VRInterface vr;
	{
		VRInterface::CreateInfo	info = {};
		info.debugEnabled = debugEnabled;
		if (!vr.Create(info))
			return -1;
	}

	uint swapchainCount = vr.GetSwapchainCount();

	while (true)
	{
		vr.Update();

		vr.StartFrame();
		for (uint s = 0; s < swapchainCount; s++)
		{
			VRInterface::SwapchainRenderInfo renderInfo = {};
			vr.StartSwapchain(s, renderInfo);
			if(renderInfo.shouldRender)
			{
				CommandBuffer* cmdBuffer = renderInfo.pCmdBuffer;
				RenderTarget* pTarget = renderInfo.pRenderTarget;
				pTarget->Bind(cmdBuffer);
				pTarget->Unbind(cmdBuffer);
			}
			vr.EndSwapchain(s);
		}
		vr.EndFrame();
	}

	return 0;
}