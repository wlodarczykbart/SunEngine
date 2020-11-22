#include "IGraphicsPipeline.h"
#include "Shader.h"
#include "RenderTarget.h"
#include "GraphicsPipeline.h"



namespace SunEngine
{

	GraphicsPipeline::GraphicsPipeline() : GraphicsObject(GraphicsObject::GRAPHICS_PIPELINE)
	{
		_apiPipeline = 0;
		_shader = 0;
	}


	GraphicsPipeline::~GraphicsPipeline()
	{
		Destroy();
	}

	bool GraphicsPipeline::Create(const CreateInfo & info)
	{
		if (!Destroy())
			return false;

		IGraphicsPipelineCreateInfo apiInfo;
		apiInfo.pShader = (IShader*)info.pShader->GetAPIHandle();
		apiInfo.settings = info.settings;

		if (!_apiPipeline)
			_apiPipeline = AllocateGraphics<IGraphicsPipeline>();

		if (!_apiPipeline->Create(apiInfo)) return false;

		_settings = info.settings;
		_shader = info.pShader;
		return true;
	}

	bool GraphicsPipeline::Destroy()
	{
		if (!GraphicsObject::Destroy())
			return false;

		_apiPipeline = 0;
		return true;
	}

	IObject * GraphicsPipeline::GetAPIHandle() const
	{
		return _apiPipeline;
	}

	Shader * GraphicsPipeline::GetShader() const
	{
		return _shader;
	}

	const PipelineSettings& GraphicsPipeline::GetSettings() const
	{
		return _settings;
	}

}