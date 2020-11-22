#pragma once

#include "IObject.h"

namespace SunEngine
{
	class IShaderBindings : public IObject
	{
	public:
		IShaderBindings();
		virtual ~IShaderBindings();

		virtual bool Create(IShader* pShader, const Vector<IShaderResource>& textureBindings, const Vector<IShaderResource>& samplerBindings) = 0;
		virtual void SetTexture(ITexture* pTexture, uint binding) = 0;
		virtual void SetSampler(ISampler* pSampler, uint binding) = 0;
		virtual void SetTextureCube(ITextureCube* pTextureCube, uint binding) = 0;
		virtual void SetTextureArray(ITextureArray* pTextureArray, uint binding) = 0;

		static IShaderBindings * Allocate(GraphicsAPI api);
	};
}
