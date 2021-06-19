#pragma once

#include "Types.h"
#include "PipelineSettings.h"
#include "SamplerSettings.h"
#include "Image.h"
#include "MemBuffer.h"

#define MAX_GRAPHICS_FIELD_LENGTH 64
#define MAX_SHADER_BUFFER_VARIABLES 16
#define MAX_GRAPHICS_API_TYPES 4
#define MAX_SUPPORTED_RENDER_TARGETS 8

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
	class IVRInterface;

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
		MSAAMode msaa;
		ITexture* colorBuffers[MAX_SUPPORTED_RENDER_TARGETS];
		ITexture* depthBuffer;
	};

	struct ISamplerCreateInfo
	{
		SamplerSettings settings;
	};

	typedef void* VRHandle;
	struct IVRInitInfo
	{
		VRHandle inInstance;
		VRHandle inSystemID;
		VRHandle inGetInstanceProcAddr;
		bool inDebugEnabled;
		VRHandle outBinding;
		VRHandle outImageHeaders;
		Vector<int64> outSupportedSwapchainFormats;
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
		VIF_UNDEFINED,
		VIF_FLOAT2,
		VIF_FLOAT3,
		VIF_FLOAT4,
	};

	struct IVertexElement
	{
		IVertexElement()
		{
			format = VIF_UNDEFINED;
			offset = 0;
			size = 0;
			semantic[0] = '\0';
		}

		IVertexInputFormat format;
		uint size;
		uint offset;
		char semantic[MAX_GRAPHICS_FIELD_LENGTH];
	};

	struct ITextureCreateInfo
	{
		struct TextureData
		{
			ImageData image;
			ImageData* pMips;
			uint mipLevels;
		};

		TextureData* images;
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
			bindingCount = 0;
			name[0] = '\0';
		}

		char name[MAX_GRAPHICS_FIELD_LENGTH];
		ShaderResourceType type;
		ShaderResourceDimension dimension;
		ShaderBindingType bindType;
		uint binding[MAX_GRAPHICS_API_TYPES];
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
		ShaderBufferVariable()
		{
			name[0] = '\0';
			size = 0;
			offset  = 0;
			numElements = 0;
		}

		char name[MAX_GRAPHICS_FIELD_LENGTH];
		ShaderDataType type;
		uint size;
		uint offset;
		uint numElements;
	};

	struct IShaderBuffer
	{
		IShaderBuffer()
		{
			size = 0;
			stages = 0;
			name[0] = '\0';
			numVariables = 0;
		}

		char name[MAX_GRAPHICS_FIELD_LENGTH];
		ShaderBindingType bindType;
		uint binding[MAX_GRAPHICS_API_TYPES];
		uint stages;
		uint size;
		uint numVariables;
		ShaderBufferVariable variables[MAX_SHADER_BUFFER_VARIABLES];
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

		MemBuffer vertexBinaries[MAX_GRAPHICS_API_TYPES];
		MemBuffer pixelBinaries[MAX_GRAPHICS_API_TYPES];
		MemBuffer geometryBinaries[MAX_GRAPHICS_API_TYPES];
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