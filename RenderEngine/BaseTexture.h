#pragma once

#include "GraphicsObject.h"

#define TEXTURE_VERSION 1

namespace SunEngine
{

	class BaseTexture : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			ImageData image;
			ImageData* pMips;
			uint mipLevels;
			bool isExternal;
		};

		BaseTexture();
		virtual ~BaseTexture();

		//bool Create(const char* tgaFilename);
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