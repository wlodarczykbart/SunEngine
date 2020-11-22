#pragma once

namespace SunEngine
{
	enum PrimitiveTopology
	{
		SE_PT_TRIANGLE_LIST,
		SE_PT_POINT_LIST,
		SE_PT_LINE_LIST,
	};

	enum CullMode
	{
		SE_CM_FRONT,
		SE_CM_BACK,
		SE_CM_NONE,
	};

	enum PolygonMode
	{
		SE_PM_FILL,
		SE_PM_LINE,
	};

	enum FrontFace
	{
		SE_FF_CLOCKWISE,
		SE_FF_COUNTER_CLOCKWISE,
	};

	enum DepthCompareOp
	{
		SE_DC_LESS, 
		SE_DC_LESS_EQUAL,
		SE_DC_EQUAL,
	};

	enum BlendFactor
	{
		SE_BF_ZERO,
		SE_BF_ONE,
		SE_BF_SRC_COLOR,
		SE_BF_DST_COLOR,
		SE_BF_SRC_ALPHA,
		SE_BF_DST_ALPHA,
		SE_BF_ONE_MINUS_SRC_ALPHA,
		SE_BF_ONE_MINUS_DST_ALPHA
	};

	enum BlendOp
	{
		SE_BO_ADD,
		SE_BO_SUBTRACT,
	};

	struct PipelineSettings
	{
		struct InputAssembly
		{
			PrimitiveTopology topology;

			bool operator == (const InputAssembly& rhs) const
			{
				return
					topology == rhs.topology;
			}

			bool operator != (const InputAssembly rhs) const { return !(*this == rhs); }
		} inputAssembly;

		struct Rasterizer
		{
			CullMode cullMode;
			PolygonMode polygonMode;
			FrontFace frontFace;
			float depthBias;
			float depthBiasClamp;
			float slopeScaledDepthBias;
			bool enableScissor;

			bool operator == (const Rasterizer& rhs) const
			{
				return
					cullMode == rhs.cullMode &&
					polygonMode == rhs.polygonMode &&
					frontFace == rhs.frontFace &&
					depthBias == rhs.depthBias &&
					depthBiasClamp == rhs.depthBiasClamp &&
					slopeScaledDepthBias == rhs.slopeScaledDepthBias &&
					enableScissor == rhs.enableScissor;
			}

			bool operator != (const Rasterizer rhs) const { return !(*this == rhs); }
		} rasterizer;

		struct DepthStencil
		{
			bool enableDepthTest;
			bool enableDepthWrite;
			DepthCompareOp depthCompareOp;

			bool operator == (const DepthStencil& rhs) const
			{
				return
					enableDepthTest == rhs.enableDepthTest &&
					enableDepthWrite == rhs.enableDepthWrite &&
					depthCompareOp == rhs.depthCompareOp;
			}

			bool operator != (const DepthStencil rhs) const { return !(*this == rhs); }
		} depthStencil;

		struct BlendState
		{
			bool enableBlending;
			BlendFactor srcColorBlendFactor;
			BlendFactor dstColorBlendFactor;
			BlendFactor srcAlphaBlendFactor;
			BlendFactor dstAlphaBlendFactor;
			BlendOp colorBlendOp;
			BlendOp alphaBlendOp;

			bool operator == (const BlendState& rhs) const
			{
				return
					enableBlending == rhs.enableBlending &&
					srcColorBlendFactor == rhs.srcColorBlendFactor &&
					dstColorBlendFactor == rhs.dstColorBlendFactor &&
					srcAlphaBlendFactor == rhs.srcAlphaBlendFactor &&
					dstAlphaBlendFactor == rhs.dstAlphaBlendFactor &&
					colorBlendOp == rhs.colorBlendOp &&
					alphaBlendOp == rhs.alphaBlendOp;
			}

			bool operator != (const BlendState rhs) const { return !(*this == rhs); }
		} blendState;

		inline void EnableAlphaBlend()
		{
			blendState.enableBlending = true;
			blendState.srcColorBlendFactor = SE_BF_SRC_ALPHA;
			blendState.dstColorBlendFactor = SE_BF_ONE_MINUS_SRC_ALPHA;
			blendState.srcAlphaBlendFactor = SE_BF_ONE;
			blendState.dstAlphaBlendFactor = SE_BF_ZERO;
			blendState.colorBlendOp = SE_BO_ADD;
			blendState.alphaBlendOp = SE_BO_ADD;

			depthStencil.enableDepthTest = true;
			depthStencil.enableDepthWrite = false;
		}

		inline void EnableAdditiveBlend()
		{
			blendState.enableBlending = true;
			blendState.srcColorBlendFactor = SE_BF_SRC_ALPHA;
			blendState.dstColorBlendFactor = SE_BF_ONE;
			blendState.srcAlphaBlendFactor = SE_BF_ONE;
			blendState.dstAlphaBlendFactor = SE_BF_ZERO;
			blendState.colorBlendOp = SE_BO_ADD;
			blendState.alphaBlendOp = SE_BO_ADD;

			depthStencil.enableDepthTest = true;
			depthStencil.enableDepthWrite = false;
		}

		PipelineSettings()
		{
			inputAssembly.topology = SE_PT_TRIANGLE_LIST;

			rasterizer.cullMode = SE_CM_BACK;
			rasterizer.frontFace = SE_FF_COUNTER_CLOCKWISE;
			rasterizer.polygonMode = SE_PM_FILL;
			rasterizer.slopeScaledDepthBias = 0.0f;
			rasterizer.depthBias = 0.0f;
			rasterizer.depthBiasClamp = 0.0f;
			rasterizer.enableScissor = false;

			depthStencil.enableDepthTest = true;
			depthStencil.enableDepthWrite = true;
			depthStencil.depthCompareOp = SE_DC_LESS;

			blendState.enableBlending = false;
			blendState.srcColorBlendFactor = blendState.srcAlphaBlendFactor = SE_BF_ONE;
			blendState.dstColorBlendFactor = blendState.dstAlphaBlendFactor = SE_BF_ZERO;
			blendState.colorBlendOp = blendState.alphaBlendOp = SE_BO_ADD;
		}

		bool operator == (const PipelineSettings& rhs) const
		{
			return
				inputAssembly == rhs.inputAssembly &&
				rasterizer == rhs.rasterizer &&
				depthStencil == rhs.depthStencil &&
				blendState == rhs.blendState;
		}

		bool operator != (const PipelineSettings rhs) const { return !(*this == rhs); }
	};

}