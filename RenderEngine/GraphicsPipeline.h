#pragma once

#include "GraphicsAPIDef.h"
#include "PipelineSettings.h"
#include "GraphicsObject.h"

#define GRAPHICS_PIPELINE_VERSION 1

namespace SunEngine
{

	class BaseShader;
	class RenderTarget;

	class GraphicsPipeline : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			CreateInfo();

			PipelineSettings settings;
			const BaseShader* pShader;
			String shaderPass;
		};

		GraphicsPipeline();
		~GraphicsPipeline();

		bool Create(const CreateInfo &info);
		bool Destroy() override;

		IObject* GetAPIHandle() const override;

		BaseShader* GetShader() const;
		const PipelineSettings& GetSettings() const;

	private:
		BaseShader* _shader;
		PipelineSettings _settings;
		IGraphicsPipeline* _apiPipeline;
	};

}