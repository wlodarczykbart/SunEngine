#pragma once

#include "Types.h"
#include "PipelineSettings.h"
#include "SamplerSettings.h"
#include "Image.h"
#include "MemBuffer.h"

namespace SunEngine
{
	enum GraphicsAPI
	{
		SE_GFX_VULKAN,
		SE_GFX_D3D11,
	};

	class IObject;
	class ICommandBuffer;
	class IDevice;
	class IGraphicsPipeline;
	class IMesh;
	class IRenderTarget;
	class ISampler;
	class IShader;
	class ISurface;
	class IUniformBuffer;
	class ITexture;
	class IShaderBindings;
	class ITextureCube;
	class ITextureArray;

	void SetGraphicsAPI(GraphicsAPI api);
	GraphicsAPI GetGraphicsAPI();

	template<typename T>
	T* AllocateGraphics();

	struct IGraphicsPipelineCreateInfo
	{
		PipelineSettings settings;
		IShader* pShader;
	};

	struct IDeviceCreateInfo
	{
		bool debugEnabled;
	};

	struct IMeshCreateInfo
	{
		uint vertexStride;
		uint numVerts;
		const void* pVerts;

		uint numIndices;
		const uint* pIndices;
	};

	struct IRenderTargetCreateInfo
	{
		uint numTargets;
		uint width;
		uint height;
		ITexture* colorBuffer;
		ITexture* depthBuffer;
	};

	struct ISamplerCreateInfo
	{
		SamplerSettings settings;
	};

	enum ShaderStage
	{
		SS_VERTEX = 1 << 0,
		SS_PIXEL = 1 << 1,
		SS_GEOMETRY = 1 << 2,
	};

	enum ShaderBindingType
	{
		SBT_CAMERA,
		SBT_OBJECT,
		SBT_LIGHT,
		SBT_ENVIRONMENT,
		SBT_SHADOW,
		SBT_SCENE,
		SBT_BONES,
		SBT_MATERIAL,
	};

	enum IVertexInputFormat
	{
		VIF_FLOAT2,
		VIF_FLOAT3,
		VIF_FLOAT4,
	};

	struct IVertexElement
	{
		IVertexInputFormat format;
		uint size;
		uint offset;
		String semantic;
	};

	struct ITextureCreateInfo
	{
		ImageData image;
		ImageData* pMips;
		uint mipLevels;
	};

	struct ITextureCubeCreateInfo
	{
		ImageData images[6];
	};

	struct ITextureArrayCreateInfo
	{
		ITextureCreateInfo* pImages;
		uint numImages;
	};

	enum ShaderResourceType
	{
		SRT_TEXTURE,
		SRT_SAMPLER,

		SRT_UNSUPPORTED = 0x777777F
	};

	enum ShaderResourceDimension
	{
		SRD_TEXTURE_2D,
		SRD_TEXTURE_CUBE,
		SRD_TEXTURE_ARRAY,
	};

	struct IShaderResource
	{
		IShaderResource()
		{
			type = SRT_UNSUPPORTED;
			dimension = SRD_TEXTURE_2D;
			stages = 0;
		}

		String name;
		ShaderResourceType type;
		ShaderResourceDimension dimension;
		ShaderBindingType bindType;
		uint binding[8];
		uint bindingCount;
		uint stages;
	};

	enum ShaderDataType
	{
		SDT_FLOAT,
		SDT_FLOAT2,
		SDT_FLOAT3,
		SDT_FLOAT4,
		SDT_MAT2,
		SDT_MAT3,
		SDT_MAT4,
		SDT_STRUCT,

		SDT_UNDEFINED,
	};

	struct ShaderBufferVariable
	{
		String Name;
		ShaderDataType Type;
		uint Size;
		uint Offset;
		uint NumElements;
	};

	struct IShaderBuffer
	{
		IShaderBuffer()
		{
			size = 0;
			stages = 0;
		}

		String name;
		ShaderBindingType bindType;
		uint binding[8];
		uint stages;
		uint size;
		Vector<ShaderBufferVariable> Variables;
	};

	struct IUniformBufferCreateInfo
	{
		uint size;
		bool isShared;
	};

	struct IShaderCreateInfo
	{
		StrMap<IShaderBuffer> buffers;
		StrMap<IShaderResource> resources;
		Vector<IVertexElement> vertexElements;

		MemBuffer vertexBinaries[8];
		MemBuffer pixelBinaries[8];
		MemBuffer geometryBinaries[8];
	};

	struct IShaderBindingCreateInfo
	{
		ShaderBindingType type;
		IShader* pShader;
		Vector<IShaderBuffer> bufferBindings;
		Vector<IShaderResource> resourceBindings;
	};

	enum ObjectBindType
	{
		IOBT_SHADER_BINDINGS,
	};

	struct IBindState
	{
		virtual ObjectBindType GetType() const = 0;
	};

	struct IShaderBindingsBindState : public IBindState
	{
		ObjectBindType GetType() const override { return IOBT_SHADER_BINDINGS; }

		Pair<String, uint> DynamicIndices[8];
	};
}