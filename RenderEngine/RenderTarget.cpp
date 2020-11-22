#include "IRenderTarget.h"
#include "BaseTexture.h"

#include "RenderTarget.h"

namespace SunEngine
{
	RenderTarget::CreateInfo::CreateInfo()
	{
		width = 0;
		height = 0;
		numTargets = 0;
		hasDepthBuffer = true;
		floatingPointColorBuffer = false;
	}

	RenderTarget::RenderTarget() : GraphicsObject(GraphicsObject::RENDER_TARGET)
	{
		_iRenderTarget = 0;
		_clearOnBind = true;

		_width = 0;
		_height = 0;
		_numTargets = 0;
		_colorTexture = 0;
		_depthTexture = 0;
	}


	RenderTarget::~RenderTarget()
	{
		Destroy();
	}

	bool RenderTarget::Create(const CreateInfo & info)
	{
		if (!Destroy())
			return false;

		if (info.numTargets)
		{
			_colorTexture = new BaseTexture();
			BaseTexture::CreateInfo texInfo = {};
			texInfo.image.Width = info.width;
			texInfo.image.Height = info.height;

			if (!info.floatingPointColorBuffer)
				texInfo.image.Flags = ImageData::COLOR_BUFFER_RGBA8;
			else
				texInfo.image.Flags = ImageData::COLOR_BUFFER_RGBA16F;
			if (!_colorTexture->Create(texInfo))
				_errStr = _colorTexture->GetErrStr();
		}

		if (info.hasDepthBuffer)
		{
			_depthTexture = new BaseTexture();
			BaseTexture::CreateInfo texInfo = {};
			texInfo.image.Width = info.width;
			texInfo.image.Height = info.height;
			texInfo.image.Flags = ImageData::DEPTH_BUFFER;
			if (!_depthTexture->Create(texInfo))
				_errStr = _depthTexture->GetErrStr();
		}

		if (!_iRenderTarget)
			_iRenderTarget = AllocateGraphics<IRenderTarget>();


		IRenderTargetCreateInfo apiInfo = {};
		apiInfo.width = info.width;
		apiInfo.height = info.height;
		apiInfo.colorBuffer = _colorTexture ? (ITexture*)_colorTexture->GetAPIHandle() : 0;
		apiInfo.depthBuffer = _depthTexture ? (ITexture*)_depthTexture->GetAPIHandle() : 0;
		apiInfo.numTargets = info.numTargets;

		if(!_iRenderTarget->Create(apiInfo)) return false;

		_width = info.width;
		_height = info.height;
		_numTargets = info.numTargets;

		SetClearColor(0.5f, 0.5f, 1.0f, 1.0f);
		return true;
	}

	bool RenderTarget::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_iRenderTarget = 0;

		if (_colorTexture)
		{
			if (!_colorTexture->Destroy())
				return false;
			delete _colorTexture;
			_colorTexture = 0;
		}

		if (_depthTexture)
		{
			if (!_depthTexture->Destroy())
				return false;
			delete _depthTexture;
			_depthTexture = 0;
		}

		return true;
	}

	IObject * RenderTarget::GetAPIHandle() const
	{
		return _iRenderTarget;
	}

	void RenderTarget::SetClearColor(const float r, const float g, const float b, const float a)
	{
		_clearColor[0] = r;
		_clearColor[1] = g;
		_clearColor[2] = b;
		_clearColor[3] = a;
		_iRenderTarget->SetClearColor(r, g, b, a);
	}

	void RenderTarget::GetClearColor(float & r, float & g, float & b, float & a) const
	{
		r = _clearColor[0];
		g = _clearColor[1];
		b = _clearColor[2];
		a = _clearColor[3];
	}

	void RenderTarget::SetViewport(const Viewport& vp)
	{
		_iRenderTarget->SetViewport(vp.x, vp.y, vp.width, vp.height);
	}

	void RenderTarget::SetClearOnBind(bool clear)
	{
		_clearOnBind = clear;
		_iRenderTarget->SetClearOnBind(clear);
	}

	bool RenderTarget::GetClearOnBind() const
	{
		return _clearOnBind;
	}

	uint RenderTarget::Width() const
	{
		return _width;
	}

	uint RenderTarget::Height() const
	{
		return _height;
	}

	BaseTexture* RenderTarget::GetColorTexture() const
	{
		return _colorTexture;
	}

	BaseTexture* RenderTarget::GetDepthTexture() const
	{
		return _depthTexture;
	}
}