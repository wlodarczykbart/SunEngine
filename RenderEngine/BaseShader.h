#pragma once

#include "ConfigFile.h"
#include "GraphicsObject.h"
#include "MemBuffer.h"

#define SHADER_VERSION 1

namespace SunEngine
{
	struct ShaderMat4
	{
		ShaderMat4();

		void Set(const void* p16Floats);

		float data[16];
	};

	struct ShaderVec4
	{
		ShaderVec4();
		void Set(float x, float y, float z, float w);
		void Set(const void* p4Floats);

		float data[4];
	};

	class UniformBuffer;
	class RenderTarget;
	class Sampler;
	class BaseTexture;
	class BaseTextureCube;
	class BaseTextureArray;

	struct CameraBufferData
	{
		ShaderMat4 ViewMatrix;
		ShaderMat4 ProjectionMatrix;
		ShaderMat4 InvViewMatrix;
		ShaderMat4 InvProjectionMatrix;
	};

	struct ObjectBufferData
	{
		ShaderMat4 WorldMatrix;
		ShaderMat4 InverseTransposeMatrix;
	};

	struct SunlightBufferData
	{
		ShaderVec4 Direction;
		ShaderVec4 ViewDirection;
		ShaderVec4 Color;
	};

	struct PointLightBufferData
	{
		ShaderVec4 Position;
		ShaderVec4 Color;
		ShaderVec4 Attenuation;
	};

	struct SpotLightBufferData
	{
		ShaderVec4 Position;
		ShaderVec4 DirectionAndAngle;
		ShaderVec4 Attenuation;
		ShaderVec4 Color;
	};

	struct FogBufferData
	{
		ShaderVec4 FogColor;
		ShaderVec4 FogControls;
	};

	struct EnvBufferData
	{
		ShaderVec4 TimeData;
		ShaderVec4 WindDir;
	};

	struct ShadowBufferData
	{
		ShaderMat4* ShadowMatrices;
	};

	struct SampleSceneBufferData
	{
		ShaderVec4 TexCoordTransform;
		ShaderVec4 TexCoordRanges; //x,z = x range    y,w = y range
	};

	struct TextureTransformBufferData
	{
		ShaderVec4* Transforms;
	};

	struct SkinnedBonesBufferData
	{
		ShaderMat4* SkinnedBones;
	};

	class ShaderStrings
	{
	public:
		static String CameraBufferName;
		static String ObjectBufferName;
		static String EnvBufferName;
		static String FogBufferName;
		static String SkinnedBoneBufferName;
		static String SunlightBufferName;
		static String PointlightBufferName;
		static String SpotlightBufferName;
		static String TextureTransformBufferName;
		static String MaterialBufferName;
		static String ShadowBufferName;

		static String SceneTextureName;
		static String SceneSamplerName;
		static String DepthTextureName;
		static String DepthSamplerName;
		static String ShadowTextureName;
		static String ShadowSamplerName;
	};

	class BaseShader;

	class ShaderBindings : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			ShaderBindingType type;
			const BaseShader* pShader;
		};

		ShaderBindings();
		~ShaderBindings() = default;

		IObject* GetAPIHandle() const override;

		bool SetUniformBuffer(const String& name, UniformBuffer* pBuffer);
		bool SetTexture(const String& name, BaseTexture* pTexture);
		bool SetSampler(const String& name, Sampler* pSampler);
		bool SetTextureCube(const String& name, BaseTextureCube* pTextureCube);
		bool SetTextureArray(const String& name, BaseTextureArray* pTextureArray);

		bool Create(const CreateInfo& info);
		bool Destroy() override;

		inline BaseShader* GetShader() const { return _shader; }
		//bool UpdateIfChanged();

		bool ContainsBuffer(const String& name) const;
		bool ContainsResource(const String& name) const;

	private:
		struct ResourceInfo
		{
			ResourceInfo();

			GraphicsObject* Resource;
			IObject* CurrentHandle;
			String Name;

			void Set(GraphicsObject* pObject);
		};

		BaseShader* _shader;
		IShaderBindings* _iBindings;
		StrMap<ResourceInfo> _resourceMap;
	};

	class BaseShader : public GraphicsObject
	{
	public:
		struct CreateInfo
		{
			StrMap<IShaderBuffer> buffers;
			StrMap<IShaderResource> resources;
			Vector<IVertexElement> vertexElements;

			MemBuffer vertexBinaries[MAX_GRAPHICS_API_TYPES];
			MemBuffer pixelBinaries[MAX_GRAPHICS_API_TYPES];
			MemBuffer geometryBinaries[MAX_GRAPHICS_API_TYPES];
		};

		BaseShader();
		~BaseShader();

		bool Create(const CreateInfo& info);
		bool Destroy() override;

		IObject* GetAPIHandle() const override;

		void GetBufferInfos(Vector<IShaderBuffer>& infos) const;
		void GetResourceInfos(Vector<IShaderResource>& infos) const;

		bool ContainsBuffer(const String& name) const;
		bool ContainsResource(const String& name) const;

	protected:

	private:

		StrMap<IShaderBuffer> _buffers;
		StrMap<IShaderResource> _resources;
		IShader* _iShader;
	};

}