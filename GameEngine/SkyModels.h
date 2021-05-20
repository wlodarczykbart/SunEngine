#pragma once

#include "Texture2D.h"
#include "Material.h"

namespace SunEngine
{
	class TextureCube;

	class SkyModel
	{
	public:
		SkyModel();
		virtual ~SkyModel();
		Material* GetMaterial() const { return _material.get(); }

		virtual void Init() = 0;
		virtual void Update(const glm::vec3& sunDirection) { (void)sunDirection; };
		virtual uint GetVertexDrawCount() const = 0;
	protected:
		UniquePtr<Material> _material;
	};

	class SkyModelSkybox : public SkyModel
	{
	public:
		SkyModelSkybox();
		void Init() override;
		uint GetVertexDrawCount() const override { return 36; }

		bool SetSkybox(TextureCube* pSkybox);
	private:
		TextureCube* _skybox;
	};

	class SkyModelArHosek : public SkyModel
	{
	public:
		void Init() override;
		void Update(const glm::vec3& sunDirection) override;
		uint GetVertexDrawCount() const override { return 6; }
	private:
		Texture2D _sphericalLookupTexture;
	};
}