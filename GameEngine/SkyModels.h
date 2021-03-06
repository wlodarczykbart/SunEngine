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

		const glm::vec3& GetSkyColor() const { return _skyColor; }
	protected:
		UniquePtr<Material> _material;
		glm::vec3 _skyColor;
	};

	class SkyModelSkybox : public SkyModel
	{
	public:
		SkyModelSkybox();
		void Init() override;

		bool SetSkybox(TextureCube* pSkybox);
	private:
		TextureCube* _skybox;
	};

	class SkyModelArHosek : public SkyModel
	{
	public:
		SkyModelArHosek();

		void Init() override;
		void Update(const glm::vec3& sunDirection) override;

		float GetTurbidity() const { return _turbidity; }
		void SetTurbidity(float value) { _turbidity = value; }

		float GetAlbedo() const { return _albedo; }
		void SetAlbedo(float value) { _albedo = value; }

		float GetIntensity() const { return _intensity; }
		void SetIntensity(float value) { _intensity = value; }

	private:
		float _turbidity;
		float _albedo;
		float _intensity;
		Vector<glm::vec3> _sampleDirections;
	};
}