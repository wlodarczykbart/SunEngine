#pragma once

#include "GraphicsObject.h"

#define RENDER_TARGET_VERSION 1

namespace SunEngine
{
	class Camera;
	class BaseTexture;

	struct Viewport
	{
		float x;
		float y;
		float width;
		float height;
	};

	class RenderTarget : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			uint width;
			uint height;
			uint numTargets;
			MSAAMode msaa;
			bool hasDepthBuffer;
			bool floatingPointColorBuffer;

			BaseTexture* pSharedDepthBuffer;

			CreateInfo();
		};

		RenderTarget();
		~RenderTarget();

		bool Create(const CreateInfo &info);
		bool Destroy() override;

		IObject* GetAPIHandle() const override;

		void SetClearColor(const float r, const float g, const float b, const float a);
		void GetClearColor(float &r, float &g, float &b, float &a) const;
		void SetViewport(const Viewport& vp);

		void SetClearOnBind(bool clear);
		bool GetClearOnBind() const;

		uint Width() const;
		uint Height() const;

		BaseTexture* GetColorTexture(uint target = 0) const;
		BaseTexture* GetDepthTexture() const;

	private:
		IRenderTarget * _iRenderTarget;
		BaseTexture* _colorTextures[MAX_SUPPORTED_RENDER_TARGETS];
		BaseTexture* _depthTexture;
		float _clearColor[4];
		uint _width;
		uint _height;
		uint _numTargets;
		bool _clearOnBind;
		bool _ownsDepthTexture;
	};

}