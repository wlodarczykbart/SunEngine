#pragma once

#include "ISurface.h"
#include "D3D11RenderOutput.h"

namespace SunEngine
{
	class D3D11CommandBuffer;

	class D3D11Surface : public D3D11RenderOutput, public ISurface
	{
	public:
		D3D11Surface();
		~D3D11Surface();

		bool Create(GraphicsWindow *window) override;
		bool Destroy() override;

		bool StartFrame(ICommandBuffer* cmdBuffer) override;
		bool SubmitFrame(ICommandBuffer* cmdBuffer) override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

		uint GetBackBufferCount() const;

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
