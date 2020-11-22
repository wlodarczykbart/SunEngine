#pragma once

#include "ISurface.h"
#include "DirectXRenderOutput.h"

namespace SunEngine
{
	class DirectXCommandBuffer;

	class DirectXSurface : public DirectXRenderOutput, public ISurface
	{
	public:
		DirectXSurface();
		~DirectXSurface();

		bool Create(GraphicsWindow *window) override;
		bool Destroy() override;

		bool StartFrame(ICommandBuffer* cmdBuffer) override;
		bool SubmitFrame(ICommandBuffer* cmdBuffer) override;

		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:

		bool createSwapchain();
		bool createRenderTargetViews();
		bool createDepthStencilView();

		GraphicsWindow* _window;

		IDXGISwapChain* _swapchain;
		ID3D11RenderTargetView* _rtv;
		ID3D11DepthStencilView* _dsv;
	};

}
