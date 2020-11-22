#pragma once

#include "Types.h"
#include "PipelineSettings.h"
#include "SamplerSettings.h"
#include "Image.h"

namespace SunEngine
{
	enum GraphicsAPI
	{
		SE_GFX_DIRECTX,
		SE_GFX_VULKAN
	};

	enum ShaderBufferBindingLocations
	{
		SBL_CAMERA_BUFFER = 0,
		SBL_OBJECT_BUFFER = 1,
		SBL_MATERIAL = 2,
		SBL_SUN_LIGHT = 3,
		SBL_POINT_LIGHT = 4,
		SBL_SPOT_LIGHT = 5,
		SBL_TEXTURE_TRANSFORM = 6,
		SBL_FOG = 7,
		SBL_SKINNED_BONES = 8,
		SBL_ENV = 9,
		SBL_SHADOW_BUFFER = 10,
		SBL_SAMPLE_SCENE = 11,

		SBL_COUNT
	};

	enum ShaderTextureBindingLocations
	{
		STL_SCENE,
		STL_DEPTH,
		STL_SHADOW,

		STL_COUNT,
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

	enum ShaderStages
	{
		SS_VERTEX = 1 << 0,
		SS_PIXEL = 1 << 1,
		SS_GEOMETRY = 1 << 2,
	};

	enum APIVertexInputFormat
	{
		VIF_FLOAT2,
		VIF_FLOAT3,
		VIF_FLOAT4,
	};

	struct APIVertexElement
	{
		String name;
		String semantic;
		APIVertexInputFormat format;
		uint size;
		uint offset;
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
		SRT_CONST_BUFFER,
		SRT_TEXTURE,
		SRT_SAMPLER,

		SRT_UNSUPPORTED = 0x777777F
	};

	enum ShaderResourceFlags
	{
		SRF_TEXTURE_2D = 1 << 0,
		SRF_TEXTURE_CUBE = 1 << 1,
		SRF_TEXTURE_ARRAY = 1 << 2,
	};

	struct IShaderResource
	{
		IShaderResource()
		{
			type = SRT_UNSUPPORTED;
			flags = 0;
			size = 0;
			stages = 0;
			binding = 0;
		}

		String name;
		ShaderResourceType type;
		uint binding;
		uint stages;
		String dataStr;
		uint size;
		uint flags;
	};

	struct IShaderCreateInfo
	{
		Vector<IShaderResource> constBuffers;
		Vector<IShaderResource> textures;
		Vector<IShaderResource> samplers;
		Vector<APIVertexElement> vertexElements;

		String vertexShaderFilename;
		String pixelShaderFilename;
		String geometryShaderFilename;
	};

	struct IUniformBufferCreateInfo
	{
		IShaderResource resource;
		IShader* pShader;
	};
}