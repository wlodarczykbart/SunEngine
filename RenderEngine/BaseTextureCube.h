#pragma once

#include "GraphicsObject.h"

namespace SunEngine
{
	class BaseTextureCube : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			ImageData images[6]; //no mip map support for cubemaps
		};


		BaseTextureCube();
		virtual ~BaseTextureCube();

		bool Create(const CreateInfo &info);
		bool Destroy() override;

		IObject* GetAPIHandle() const override;

		uint GetWidth() const;
		uint GetHeight() const;

	private:
		ITextureCube* _iTexture;
		uint _width;
		uint _height;
	};

}