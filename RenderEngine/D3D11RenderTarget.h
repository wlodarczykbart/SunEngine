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
		void SetClearColor(const float r, const float g, const float b, const float a) override;
		void SetClearOnBind(bool clear) override;
		void SetViewport(float x, float y, float width, float height) override;

		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;

	private:
		friend class D3D11Shader;
		bool createViews(ITexture* const* pColorTextures, ITexture* pDepthTex);

		uint _numTargets;
		uint _width;
		uint _height;

		ID3D11RenderTargetView* _rtv[IRenderTargetCreateInfo::MAX_TARGETS];
		ID3D11DepthStencilView* _dsv;

		D3D11_VIEWPORT _viewport;

		FLOAT _clearColor[4];
		bool _clearOnBind;
	};

}
