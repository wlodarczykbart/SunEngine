#pragma once

#include "GraphicsObject.h"

namespace SunEngine
{
	class BaseTexture : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			CreateInfo();

			struct TextureData
			{
				ImageData image;
				ImageData* pMips;
				uint mipLevels;
			};

			TextureData* pImages;
			uint numImages;
			bool isExternal;
		};

		BaseTexture();
		virtual ~BaseTexture();

		bool Create(const CreateInfo &info);
		bool Destroy() override;

		IObject* GetAPIHandle() const override;

		uint GetWidth() const;
		uint GetHeight() const;
	private:
		ITexture* _iTexture;
		uint _width;
		uint _height;
	};

}