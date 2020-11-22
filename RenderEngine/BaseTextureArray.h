#pragma once

#include "BaseTexture.h"

namespace SunEngine
{
	class BaseTextureArray : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			BaseTexture::CreateInfo* pImages;
			uint numImages;
		};

		BaseTextureArray();
		virtual ~BaseTextureArray();

		bool Create(const CreateInfo &info);
		bool Destroy() override;

		IObject* GetAPIHandle() const override;

		uint GetWidth() const;
		uint GetHeight() const;

	private:
		ITextureArray* _iTexture;
		uint _width;
		uint _height;
	};

}