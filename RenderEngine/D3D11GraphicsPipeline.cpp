#include "D3D11Shader.h"
#include "D3D11RenderOutput.h"
#include "D3D11CommandBuffer.h"
#include "D3D11GraphicsPipeline.h"

namespace SunEngine
{
	Map<PrimitiveTopology, D3D11_PRIMITIVE_TOPOLOGY> PrimTopologyMap =
	{
		{ SE_PT_TRIANGLE_LIST, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, },
		{ SE_PT_POINT_LIST, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST, },
		{ SE_PT_LINE_LIST, D3D11_PRIMITIVE_TOPOLOGY_LINELIST, },
	};

	Map<CullMode, D3D11_CULL_MODE> CullModeMap =
	{
		{ SE_CM_FRONT, D3D11_CULL_FRONT }, 
		{ SE_CM_BACK, D3D11_CULL_BACK },
		{ SE_CM_NONE, D3D11_CULL_NONE },
	};

	Map<PolygonMode, D3D11_FILL_MODE> PolygonModeMap =
	{
		{ SE_PM_FILL, D3D11_FILL_SOLID }, 
		{ SE_PM_LINE, D3D11_FILL_WIREFRAME }, 
	};

	Map<DepthCompareOp, D3D11_COMPARISON_FUNC> DepthCompreOpMap =
	{
		{ SE_DC_LESS, D3D11_COMPARISON_LESS },
		{ SE_DC_LESS_EQUAL, D3D11_COMPARISON_LESS_EQUAL },
		{ SE_DC_EQUAL, D3D11_COMPARISON_EQUAL },
		{ SE_DC_ALWAYS, D3D11_COMPARISON_ALWAYS },
		{ SE_DC_GREATER, D3D11_COMPARISON_GREATER },
		{ SE_DC_NOT_EQUAL, D3D11_COMPARISON_NOT_EQUAL },
	};

	Map<BlendFactor, D3D11_BLEND> BlendFactorMap =
	{
		{ SE_BF_ZERO,					D3D11_BLEND_ZERO },
		{ SE_BF_ONE,					D3D11_BLEND_ONE },
		{ SE_BF_SRC_COLOR,				D3D11_BLEND_SRC_COLOR },
		{ SE_BF_DST_COLOR,				D3D11_BLEND_DEST_COLOR },
		{ SE_BF_SRC_ALPHA,				D3D11_BLEND_SRC_ALPHA },
		{ SE_BF_DST_ALPHA,				D3D11_BLEND_DEST_ALPHA },
		{ SE_BF_ONE_MINUS_SRC_ALPHA,	D3D11_BLEND_INV_SRC_ALPHA },
		{ SE_BF_ONE_MINUS_DST_ALPHA,	D3D11_BLEND_INV_DEST_ALPHA },
	};

	Map<BlendOp, D3D11_BLEND_OP> BlendOpMap =
	{
		{ SE_BO_ADD, D3D11_BLEND_OP_ADD, },
		{ SE_BO_SUBTRACT, D3D11_BLEND_OP_SUBTRACT, },
	};

	D3D11GraphicsPipeline::D3D11GraphicsPipeline()
	{
		_rasterizerState = 0;
		_depthStencilState = 0;
		_blendState = 0;
	}


	D3D11GraphicsPipeline::~D3D11GraphicsPipeline()
	{
		Destroy();
	}

	bool D3D11GraphicsPipeline::Create(const IGraphicsPipelineCreateInfo & info)
	{
		D3D11Shader *pShader = (D3D11Shader*)info.pShader;

		PipelineSettings settings = info.settings;

		_primitiveTopology = PrimTopologyMap[settings.inputAssembly.topology];

		D3D11_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.CullMode = CullModeMap[info.settings.rasterizer.cullMode];
		rasterizerDesc.FillMode = PolygonModeMap[info.settings.rasterizer.polygonMode];
		rasterizerDesc.FrontCounterClockwise = info.settings.rasterizer.frontFace == SE_FF_COUNTER_CLOCKWISE;
		rasterizerDesc.DepthBias = info.settings.rasterizer.depthBias;
		rasterizerDesc.DepthBiasClamp = info.settings.rasterizer.depthBiasClamp;
		rasterizerDesc.SlopeScaledDepthBias = info.settings.rasterizer.slopeScaledDepthBias;
		rasterizerDesc.ScissorEnable = info.settings.rasterizer.enableScissor;
		if (!_device->CreateRasterizerState(rasterizerDesc, &_rasterizerState)) return false;

		D3D11_DEPTH_STENCIL_DESC depthDesc = {};
		depthDesc.DepthEnable = settings.depthStencil.enableDepthTest;
		depthDesc.DepthWriteMask = settings.depthStencil.enableDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = DepthCompreOpMap[settings.depthStencil.depthCompareOp];
		depthDesc.StencilEnable = FALSE;
		depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		depthDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		depthDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depthDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthDesc.BackFace = depthDesc.FrontFace;
		if (!_device->CreateDepthStencilState(depthDesc, &_depthStencilState)) return false;

		D3D11_BLEND_DESC blendDesc = {};
		for (uint i = 0; i < ARRAYSIZE(blendDesc.RenderTarget); i++)
		{
			D3D11_RENDER_TARGET_BLEND_DESC blend = {};
			blend.BlendEnable = settings.blendState.enableBlending;
			blend.BlendOp = BlendOpMap[settings.blendState.colorBlendOp];
			blend.BlendOpAlpha = BlendOpMap[settings.blendState.alphaBlendOp];

			blend.SrcBlend = BlendFactorMap[settings.blendState.srcColorBlendFactor];
			blend.SrcBlendAlpha = BlendFactorMap[settings.blendState.srcAlphaBlendFactor];

			blend.DestBlend = BlendFactorMap[settings.blendState.dstColorBlendFactor];
			blend.DestBlendAlpha = BlendFactorMap[settings.blendState.dstAlphaBlendFactor];

			blend.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			blendDesc.RenderTarget[i] = blend;
		}
		if (!_device->CreateBlendState(blendDesc, &_blendState)) return false;

		return true;
	}

	void D3D11GraphicsPipeline::Bind(ICommandBuffer * cmdBuffer, IBindState*)
	{
		D3D11CommandBuffer* dxCmd = (D3D11CommandBuffer*)cmdBuffer;
		dxCmd->SetPrimitiveTopology(_primitiveTopology);
		dxCmd->BindRasterizerState(_rasterizerState);
		dxCmd->BindDepthStencilState(_depthStencilState);
		dxCmd->BindBlendState(_blendState);
	}

	void D3D11GraphicsPipeline::Unbind(ICommandBuffer * cmdBuffer)
	{
		//????_shader->Unbind(cmdBuffer);
	}

	bool D3D11GraphicsPipeline::Destroy()
	{
		COM_RELEASE(_depthStencilState);
		COM_RELEASE(_rasterizerState);
		COM_RELEASE(_blendState);
		return true;
	}
}