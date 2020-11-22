#pragma once

#include "IRenderTarget.h"
#include "DirectXRenderOutput.h"

namespace SunEngine
{

	class DirectXRenderTarget : public DirectXRenderOutput, public IRenderTarget
	{
	public:
		DirectXRenderTarget();
		~DirectXRenderTarget();

		bool Create(const IRenderTargetCreateInfo &info) override;
		bool Destroy() override;
		void SetClearColor(const float r, const float g, const float b, const float a) override;
		void SetClearOnBind(bool clear) override;
		void SetViewport(float x, float y, float width, float height) override;

		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		friend class DirectXShader;
		bool createViews(ITexture* pColorTex, ITexture* pDepthTex);

		uint _numTargets;
		uint _width;
		uint _height;

		ID3D11RenderTargetView* _rtv;
		ID3D11DepthStencilView* _dsv;

		D3D11_VIEWPORT _viewport;

		FLOAT _clearColor[4];
		bool _clearOnBind;
	};

}
