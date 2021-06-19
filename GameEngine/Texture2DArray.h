#pragma once

#include "BaseTexture.h"
#include "Texture2D.h"

namespace SunEngine
{
	class Texture2DArray : public GPUResource<BaseTexture>
	{
	public:
		Texture2DArray();
		~Texture2DArray();

		bool RegisterToGPU() override;

		void SetWidth(uint width) { _width = width; }
		uint GetWidth() const { return _width; }

		void SetHeight(uint height) { _height = height; }
		uint GetHeight() const { return _height; }

		bool AddTexture(Texture2D* pTexture);
		bool SetTexture(uint index, Texture2D* pTexture);

		bool GenerateMips(bool threaded);
		bool Compress();
		void SetSRGB();

		void SetPixel(uint x, uint y, uint i, const Pixel& color);
		void SetPixel(uint x, uint y, uint i, const glm::vec4& color);

	private:
		struct TextureEntry
		{
			Texture2D* pSrc;
			Texture2D  texture; //a texture that is scaled to match the texture array width/height
		};

		Vector<UniquePtr<TextureEntry>> _textures;
		uint _width;
		uint _height;
	};
}