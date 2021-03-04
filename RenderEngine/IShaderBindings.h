#pragma once

#include "IObject.h"

namespace SunEngine
{
	class IShaderBindings : public IObject
	{
	public:
		IShaderBindings();
		virtual ~IShaderBindings();

		virtual bool Create(const IShaderBindingCreateInfo& createInfo) = 0;
		virtual void SetUniformBuffer(IUniformBuffer* pBuffer, const String& name) = 0;
		virtual void SetTexture(ITexture* pTexture, const String& name) = 0;
		virtual void SetSampler(ISampler* pSampler, const String& name) = 0;
		virtual void SetTextureCube(ITextureCube* pTextureCube, const String& name) = 0;
		virtual void SetTextureArray(ITextureArray* pTextureArray, const String& name) = 0;

		static IShaderBindings * Allocate(GraphicsAPI api);
	};
}
