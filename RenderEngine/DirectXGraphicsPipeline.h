#pragma once

#include "DirectXObject.h"
#include "IGraphicsPipeline.h"
#include "PipelineSettings.h"

namespace SunEngine
{
	class DirectXRenderOutput;
	class DirectXShader;

	class DirectXGraphicsPipeline : public DirectXObject, public IGraphicsPipeline
	{
	public:
		DirectXGraphicsPipeline();
		~DirectXGraphicsPipeline();

		bool Create(const IGraphicsPipelineCreateInfo &info) override;
		void Bind(ICommandBuffer* cmdBuffer) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
		bool Destroy() override;

	private:
		D3D11_PRIMITIVE_TOPOLOGY _primitiveTopology;
		ID3D11RasterizerState* _rasterizerState;
		ID3D11DepthStencilState* _depthStencilState;
		ID3D11BlendState* _blendState;
	};

}
