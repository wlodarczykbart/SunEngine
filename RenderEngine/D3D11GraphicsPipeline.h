#pragma once

#include "D3D11Object.h"
#include "IGraphicsPipeline.h"
#include "PipelineSettings.h"

namespace SunEngine
{
	class D3D11RenderOutput;
	class D3D11Shader;

	class D3D11GraphicsPipeline : public D3D11Object, public IGraphicsPipeline
	{
	public:
		D3D11GraphicsPipeline();
		~D3D11GraphicsPipeline();

		bool Create(const IGraphicsPipelineCreateInfo &info) override;
		void Bind(ICommandBuffer* cmdBuffer, IBindState* pBindState) override;
		void Unbind(ICommandBuffer* cmdBuffer) override;
		bool Destroy() override;

	private:
		D3D11_PRIMITIVE_TOPOLOGY _primitiveTopology;
		ID3D11RasterizerState* _rasterizerState;
		ID3D11DepthStencilState* _depthStencilState;
		ID3D11BlendState* _blendState;
	};

}
