#include "IGraphicsPipeline.h"
#include "BaseShader.h"
#include "RenderTarget.h"
#include "GraphicsPipeline.h"



namespace SunEngine
{
	GraphicsPipeline::CreateInfo::CreateInfo()
	{
		shaderPass = ShaderStrings::DefaultShaderPassName;
	}

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
		apiInfo.pShader = info.pShader->GetShaderPassShader(info.shaderPass);
		if (!apiInfo.pShader)
			return false;
		apiInfo.settings = info.settings;

		if (!_apiPipeline)
			_apiPipeline = AllocateGraphics<IGraphicsPipeline>();

		if (!_apiPipeline->Create(apiInfo)) return false;

		_settings = info.settings;
		_shader = (BaseShader*)info.pShader;
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

	BaseShader* GraphicsPipeline::GetShader() const
	{
		return _shader;
	}

	const PipelineSettings& GraphicsPipeline::GetSettings() const
	{
		return _settings;
	}

}