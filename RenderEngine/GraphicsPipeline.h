#pragma once

#include "GraphicsAPIDef.h"
#include "PipelineSettings.h"
#include "GraphicsObject.h"

#define GRAPHICS_PIPELINE_VERSION 1

namespace SunEngine
{

	class Shader;
	class RenderTarget;

	class GraphicsPipeline : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			PipelineSettings settings;
			Shader *pShader;
		};

		GraphicsPipeline();
		~GraphicsPipeline();

		bool Create(const CreateInfo &info);
		bool Destroy() override;

		IObject* GetAPIHandle() const override;

		Shader* GetShader() const;
		const PipelineSettings& GetSettings() const;

	private:
		Shader* _shader;
		PipelineSettings _settings;
		IGraphicsPipeline* _apiPipeline;
	};

}