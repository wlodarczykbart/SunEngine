#pragma once

#include "IRenderTarget.h"
#include "D3D11RenderOutput.h"

namespace SunEngine
{

	class D3D11RenderTarget : public D3D11RenderOutput, public IRenderTarget
	{
	public:
		D3D11RenderTarget();
		~D3D11RenderTarget();

		bool Create(const IRenderTargetCreateInfo &info) override;
		bool Destroy() override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		struct RenderLayer
		{
			RenderLayer();
			ID3D11RenderTargetView* rtv[MAX_SUPPORTED_RENDER_TARGETS];
			ID3D11DepthStencilView* dsv;
		};

		friend class D3D11Shader;
		bool createViews(ITexture* const* pColorTextures, ITexture* pDepthTex);

		uint _numTargets;
		uint _width;
		uint _height;

		Vector<RenderLayer> _layers;
	};

}
