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
		pSharedDepthBuffer = 0;
		msaa = SE_MSAA_OFF;
		numLayers = 1;

		for (uint i = 0; i < MAX_SUPPORTED_RENDER_TARGETS; i++)
			pSharedColorBuffers[i] = 0;
	}

	RenderTarget::RenderTarget() : GraphicsObject(GraphicsObject::RENDER_TARGET)
	{
		_iRenderTarget = 0;
		_clearOnBind = true;

		_width = 0;
		_height = 0;
		_numTargets = 0;
		_depthTexture = 0;

		_clearColor[0] = 0.0f;
		_clearColor[1] = 0.0f;
		_clearColor[2] = 0.0f;
		_clearColor[3] = 1.0f;

		for (uint i = 0; i < MAX_SUPPORTED_RENDER_TARGETS; i++)
			_colorTextures[i] = 0;

		_depthTextureOwnership = false;
		_colorTextureOwnership = 0;
	}


	RenderTarget::~RenderTarget()
	{
		Destroy();
	}

	bool RenderTarget::Create(const CreateInfo & info)
	{
		if (!Destroy())
			return false;

		uint msaaFlags = 0;
		if (info.msaa == SE_MSAA_2) msaaFlags |= ImageData::MULTI_SAMPLES_2;
		else if (info.msaa == SE_MSAA_4) msaaFlags |= ImageData::MULTI_SAMPLES_4;
		else if (info.msaa == SE_MSAA_8) msaaFlags |= ImageData::MULTI_SAMPLES_8;

		for (uint i = 0; i < info.numTargets; i++)
		{
			BaseTexture* pTexture = 0;
			
			if (info.pSharedColorBuffers[i] == 0)
			{
				pTexture = new BaseTexture();

				Vector<BaseTexture::CreateInfo::TextureData> colorTexData;
				colorTexData.resize(info.numLayers);
				for (uint j = 0; j < info.numLayers; j++)
				{
					BaseTexture::CreateInfo::TextureData texData = {};

					texData.image.Width = info.width;
					texData.image.Height = info.height;

					if (!info.floatingPointColorBuffer)
						texData.image.Flags = ImageData::COLOR_BUFFER_RGBA8;
					else
						texData.image.Flags = ImageData::COLOR_BUFFER_RGBA16F;

					texData.image.Flags |= msaaFlags;
					colorTexData[j] = texData;
				}

				BaseTexture::CreateInfo texInfo = {};
				texInfo.numImages = info.numLayers;
				texInfo.pImages = colorTexData.data();
				if (!pTexture->Create(texInfo))
					_errStr = pTexture->GetErrStr();

				_colorTextureOwnership |= 1 << i;
			}
			else
			{
				pTexture = info.pSharedColorBuffers[i];
			}

			_colorTextures[i] = pTexture;
		}

		if (info.hasDepthBuffer)
		{
			if (!info.pSharedDepthBuffer)
			{
				_depthTexture = new BaseTexture();

				Vector<BaseTexture::CreateInfo::TextureData> depthTexData;
				depthTexData.resize(info.numLayers);
				for (uint i = 0; i < info.numLayers; i++)
				{
					BaseTexture::CreateInfo::TextureData texData = {};
					texData.image.Width = info.width;
					texData.image.Height = info.height;
					texData.image.Flags = ImageData::DEPTH_BUFFER;
					texData.image.Flags |= msaaFlags;
					depthTexData[i] = texData;
				}

				BaseTexture::CreateInfo texInfo = {};
				texInfo.numImages = info.numLayers;
				texInfo.pImages = depthTexData.data();
				if (!_depthTexture->Create(texInfo))
					_errStr = _depthTexture->GetErrStr();

				_depthTextureOwnership = true;
			}
			else
			{
				_depthTexture = info.pSharedDepthBuffer;
			}
		}

		if (!_iRenderTarget)
			_iRenderTarget = AllocateGraphics<IRenderTarget>();


		IRenderTargetCreateInfo apiInfo = {};
		apiInfo.width = info.width;
		apiInfo.height = info.height;
		for (uint i = 0; i < info.numTargets; i++)
			apiInfo.colorBuffers[i] = (ITexture*)_colorTextures[i]->GetAPIHandle();
		apiInfo.depthBuffer = _depthTexture ? (ITexture*)_depthTexture->GetAPIHandle() : 0;
		apiInfo.numTargets = info.numTargets;
		apiInfo.msaa = info.msaa;
		apiInfo.numLayers = info.numLayers;

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

		for (uint i = 0; i < _numTargets; i++)
		{
			if (_colorTextureOwnership & (1 << i))
			{
				if (!_colorTextures[i]->Destroy())
					return false;
				delete _colorTextures[i];
			}
		}

		if (_depthTexture)
		{
			if (_depthTextureOwnership)
			{
				if (!_depthTexture->Destroy())
					return false;
				delete _depthTexture;
			}
			_depthTexture = 0;
		}

		_numTargets = 0;
		_depthTextureOwnership = true;
		_colorTextureOwnership = 0;

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
	}

	void RenderTarget::GetClearColor(float & r, float & g, float & b, float & a) const
	{
		r = _clearColor[0];
		g = _clearColor[1];
		b = _clearColor[2];
		a = _clearColor[3];
	}

	void RenderTarget::SetClearOnBind(bool clear)
	{
		_clearOnBind = clear;
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

	BaseTexture* RenderTarget::GetColorTexture(uint target) const
	{
		if(target < MAX_SUPPORTED_RENDER_TARGETS)
			return _colorTextures[target];

		return 0;
	}

	BaseTexture* RenderTarget::GetDepthTexture() const
	{
		return _depthTexture;
	}

	bool RenderTarget::Bind(CommandBuffer* cmdBuffer, IBindState* pBindState)
	{
		if (pBindState && pBindState->GetType() == IOBT_RENDER_TARGET)
			return GraphicsObject::Bind(cmdBuffer, pBindState);

		IRenderTargetBindState state = {};
		state.clearColor[0] = _clearColor[0];
		state.clearColor[1] = _clearColor[1];
		state.clearColor[2] = _clearColor[2];
		state.clearColor[3] = _clearColor[3];
		state.clearOnBind = _clearOnBind;
		state.layer = 0;

		return GraphicsObject::Bind(cmdBuffer, &state);
	}

	bool RenderTarget::BindLayer(CommandBuffer* cmdBuffer, uint layer)
	{
		IRenderTargetBindState state = {};
		state.clearColor[0] = _clearColor[0];
		state.clearColor[1] = _clearColor[1];
		state.clearColor[2] = _clearColor[2];
		state.clearColor[3] = _clearColor[3];
		state.clearOnBind = _clearOnBind;
		state.layer = layer;

		return GraphicsObject::Bind(cmdBuffer, &state);
	}
}