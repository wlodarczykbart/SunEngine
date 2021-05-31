#include "FileBase.h"
#include "GraphicsContext.h"
#include "GraphicsWindow.h"
#include "Surface.h"
#include "BaseMesh.h"
#include "BaseShader.h"
#include "GraphicsPipeline.h"
#include "CommandBuffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "StringUtil.h"
#include "RenderTarget.h"

#if 1

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using namespace SunEngine;

struct ShaderBindingData
{
	UniformBuffer ubo;
	ShaderBindings bindings;
	IShaderBuffer bufferInfo;
};

enum CSMFitMode
{
	FIT_TO_CASCADES,
	FIT_TO_SCENE,
};

enum CSMFitNearFarMode
{
	FIT_NEARFAR_AABB,
	FIT_NEARFAR_SCENE_AABB,
	FIT_NEARFAR_PANCAKING,
};

static const glm::vec4 g_vFLTMAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
static const glm::vec4 g_vFLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
static const glm::vec4 g_vHalfVector = { 0.5f, 0.5f, 0.5f, 0.5f };
static const glm::vec4 g_vMultiplySetzwToZero = { 1.0f, 1.0f, 0.0f, 0.0f };
static const glm::vec4 g_vZero = { 0.0f, 0.0f, 0.0f, 0.0f };

#define MAX_CASCADES 8

glm::vec4 m_vSceneAABBMin, m_vSceneAABBMax;
INT m_iCascadePartitionsZeroToOne[MAX_CASCADES];
INT m_iCascadePartitionsMax;
glm::mat4 m_matShadowProj[MAX_CASCADES];
FLOAT m_fCascadePartitionsFrustum[MAX_CASCADES];

struct
{
	INT m_nCascadeLevels;
	INT m_iBufferSize;
} m_CopyOfCascadeConfig;

CSMFitMode m_eSelectedCascadesFit = FIT_TO_CASCADES;
CSMFitNearFarMode m_eSelectedNearFarFit = FIT_NEARFAR_SCENE_AABB;
const INT m_iPCFBlurSize = 3;

namespace glm
{
	float vec4GetX(const vec4& v)
	{
		return v.x;
	}

	float vec4GetY(const vec4& v)
	{
		return v.y;
	}

	float vec4GetZ(const vec4& v)
	{
		return v.z;
	}

	float vec4GetByIndex(const vec4& v, int index)
	{
		return v[index];
	}

	vec4 vec4Multiply(const vec4& v0, const vec4& v1)
	{
		return v0 * v1;
	}

	vec4 vec4Add(const vec4& v0, const vec4& v1)
	{
		return v0 + v1;
	}

	vec4 vec4Subtract(const vec4& v0, const vec4& v1)
	{
		return v0 - v1;
	}

	vec4 vec44Transform(const vec4& v0, const glm::mat4& m0)
	{
		return m0 * v0;
	}

	mat4 mat4RotationX(float angle)
	{
		return glm::rotate(angle, vec3(1.0f, 0.0f, 0.0f));
	}

	mat4 mat4RotationY(float angle)
	{
		return glm::rotate(angle, vec3(0.0f, 1.0f, 0.0f));
	}

	vec4 vec4Select(const vec4& v0, const vec4& v1, const uvec4& s)
	{
		vec4 result;
		for (uint i = 0; i < 4; i++)
			result[i] = s[i] == 0 ? v0[i] : v1[i];
		return result;
	}

	vec4 vec43Length(const vec4& v)
	{
		return vec4(glm::length(glm::vec3(v)));
	}

	vec4 vec4Reciprocal(const vec4& v)
	{
		return 1.0f / v;
	}

	vec4 vec4SplatZ(const vec4& v)
	{
		return vec4(v.z);
	}

	vec4 vec4SplatW(const vec4& v)
	{
		return vec4(v.w);
	}
};

struct BoundingBox
{
	glm::vec4 Center;
	glm::vec4 Extents;

	static void CreateFromPoints(BoundingBox& box, uint nPoints, const glm::vec3* points, uint stride);
	static void CreateFromPoints(BoundingBox& box, const glm::vec4& min, const glm::vec4& max);

	void Transform(BoundingBox& box, const glm::mat4& mtx) const
	{
		glm::vec3 corners[8];
		GetCorners(corners);

		glm::vec3 vmin = g_vFLTMAX;
		glm::vec3 vmax = g_vFLTMIN;

		for (uint i = 0; i < 8; i++)
		{
			glm::vec3 point = glm::vec3(mtx * glm::vec4(corners[i], 1.0f));
			vmin = glm::min(point, vmin);
			vmax = glm::max(point, vmax);
		}

		CreateFromPoints(box, glm::vec4(vmin, 1.0f), glm::vec4(vmax, 1.0f));
	}

	void GetCorners(glm::vec3* corners) const
	{
		static const glm::vec4 g_BoxOffset[8] =
		{
			{ -1.0f, -1.0f,  1.0f, 0.0f},
			{  1.0f, -1.0f,  1.0f, 0.0f},
			{  1.0f,  1.0f,  1.0f, 0.0f},
			{ -1.0f,  1.0f,  1.0f, 0.0f},
			{ -1.0f, -1.0f, -1.0f, 0.0f},
			{  1.0f, -1.0f, -1.0f, 0.0f},
			{  1.0f,  1.0f, -1.0f, 0.0f},
			{ -1.0f,  1.0f, -1.0f, 0.0f},
		};


		for (size_t i = 0; i < 8; ++i)
		{
			corners[i] = Extents * g_BoxOffset[i] + Center;
		}

		//glm::vec3 vmin = Center - Extents;
		//glm::vec3 vmax = Center + Extents;

		//corners[0] = vmin;
		//corners[1] = vmax;
		//corners[2] = glm::vec3(vmax.x, vmin.y, vmin.z);
		//corners[3] = glm::vec3(vmin.x, vmax.y, vmin.z);
		//corners[4] = glm::vec3(vmax.x, vmin.y, vmax.z);
		//corners[5] = glm::vec3(vmin.x, vmax.y, vmax.z);
		//corners[6] = glm::vec3(vmin.x, vmin.y, vmax.z);
		//corners[7] = glm::vec3(vmax.x, vmax.y, vmin.z);
	}
};

void BoundingBox::CreateFromPoints(BoundingBox& box, uint nPoints, const glm::vec3* points, uint)
{
	glm::vec3 vmin = g_vFLTMAX;
	glm::vec3 vmax = g_vFLTMIN;

	for (uint i = 0; i < nPoints; i++)
	{
		vmin = glm::min(points[i], vmin);
		vmax = glm::max(points[i], vmax);
	}

	CreateFromPoints(box, glm::vec4(vmin, 1.0f), glm::vec4(vmax, 1.0f));
}

void BoundingBox::CreateFromPoints(BoundingBox& box, const glm::vec4& vmin, const glm::vec4& vmax)
{
	box.Center = (vmin + vmax) * 0.5f;
	box.Extents = (vmax - vmin) * 0.5f;
}

struct BoundingFrustum
{
	BoundingFrustum(const glm::mat4& Projection)
	{
		// Corners of the projection frustum in homogenous space.
		static glm::vec4 HomogenousPoints[6] =
		{
			{  1.0f,  0.0f, 1.0f, 1.0f },   // right (at far plane)
			{ -1.0f,  0.0f, 1.0f, 1.0f },   // left
			{  0.0f,  1.0f, 1.0f, 1.0f },   // top
			{  0.0f, -1.0f, 1.0f, 1.0f },   // bottom

			{ 0.0f, 0.0f, 0.0f, 1.0f },     // near
			{ 0.0f, 0.0f, 1.0f, 1.0f }      // far
		};

		glm::mat4 matInverse = glm::inverse(Projection);

		// Compute the frustum corners in world space.
		glm::vec4 Points[6];

		for (size_t i = 0; i < 6; ++i)
		{
			// Transform point.
			Points[i] = matInverse * HomogenousPoints[i];
		}

		// Compute the slopes.
		Points[0] = glm::vec4Multiply(Points[0], glm::vec4Reciprocal(glm::vec4SplatZ(Points[0])));
		Points[1] = glm::vec4Multiply(Points[1], glm::vec4Reciprocal(glm::vec4SplatZ(Points[1])));
		Points[2] = glm::vec4Multiply(Points[2], glm::vec4Reciprocal(glm::vec4SplatZ(Points[2])));
		Points[3] = glm::vec4Multiply(Points[3], glm::vec4Reciprocal(glm::vec4SplatZ(Points[3])));

		RightSlope = glm::vec4GetX(Points[0]);
		LeftSlope = glm::vec4GetX(Points[1]);
		TopSlope = glm::vec4GetY(Points[2]);
		BottomSlope = glm::vec4GetY(Points[3]);

		// Compute near and far.
		Points[4] = glm::vec4Multiply(Points[4], glm::vec4Reciprocal(glm::vec4SplatW(Points[4])));
		Points[5] = glm::vec4Multiply(Points[5], glm::vec4Reciprocal(glm::vec4SplatW(Points[5])));

		Near = glm::vec4GetZ(Points[4]);
		Far = glm::vec4GetZ(Points[5]);
	}

	float LeftSlope;
	float RightSlope;
	float BottomSlope;
	float TopSlope;
	float Near;
	float Far;
};

void UpdateShadows(FLOAT nearClip, FLOAT farClip, const glm::mat4& matViewCameraProjection, const glm::mat4& matViewCameraView, const glm::mat4& matLightCameraView);

void FillShaderBufferInfo(ShaderBindingType type, IShaderBuffer& buffer)
{
	buffer = {};
	buffer.bindType = type;

	struct ShaderVarDef
	{
		ShaderVarDef()
		{
			type = SDT_UNDEFINED;
			numElements = 1;
		}

		ShaderVarDef(const char* name, ShaderDataType type) : ShaderVarDef()
		{
			this->name = name;
			this->type = type;
		}

		ShaderVarDef(const char* name, ShaderDataType type, uint numElements) : ShaderVarDef(name, type)
		{
			this->numElements = numElements;
		}

		String name;
		ShaderDataType type;
		uint numElements;
	};

	Vector<ShaderVarDef> variables;

	String bufferName;
	if (type == SBT_CAMERA)
	{
		bufferName = "CameraBuffer";
		buffer.binding[SE_GFX_D3D11] = 0;
		buffer.binding[SE_GFX_VULKAN] = 0;
		buffer.stages = SS_VERTEX;

		variables =
		{
			ShaderVarDef("ViewProjMatrix", SDT_MAT4),
			ShaderVarDef("ViewMatrix", SDT_MAT4)
		};
	}
	else if (type == SBT_OBJECT)
	{
		bufferName = "ObjectBuffer";
		buffer.binding[SE_GFX_D3D11] = 1;
		buffer.binding[SE_GFX_VULKAN] = 0;
		buffer.stages = SS_VERTEX;

		variables =
		{
			ShaderVarDef("WorldMatirx", SDT_MAT4)
		};
	}
	else if (type == SBT_SHADOW)
	{
		bufferName = "ShadowBuffer";
		buffer.binding[SE_GFX_D3D11] = 2;
		buffer.binding[SE_GFX_VULKAN] = 0;
		buffer.stages = SS_VERTEX | SS_PIXEL;

		variables =
		{
			ShaderVarDef("ShadowMatrices", SDT_MAT4, MAX_CASCADES),
			ShaderVarDef("ShadowSplitDepths", SDT_MAT4)
		};
	}

	sprintf_s(buffer.name, bufferName.c_str());
	buffer.numVariables = variables.size();
	uint offset = 0;
	for (uint i = 0; i < variables.size(); i++)
	{
		sprintf_s(buffer.variables[i].name, variables[i].name.c_str());
		buffer.variables[i].type = variables[i].type;
		switch (variables[i].type)
		{
		case SDT_FLOAT: buffer.variables[i].size = sizeof(float); break;
		case SDT_FLOAT2: buffer.variables[i].size = sizeof(glm::vec2); break;
		case SDT_FLOAT3: buffer.variables[i].size = sizeof(glm::vec3); break;
		case SDT_FLOAT4: buffer.variables[i].size = sizeof(glm::vec4); break;
		case SDT_MAT2: buffer.variables[i].size = sizeof(glm::vec2) * 2; break;
		case SDT_MAT3: buffer.variables[i].size = sizeof(glm::vec3) * 3; break;
		case SDT_MAT4: buffer.variables[i].size = sizeof(glm::vec4) * 4; break;
		default:
			break;
		}

		buffer.variables[i].size *= variables[i].numElements;
		buffer.variables[i].numElements = variables[i].numElements;
		buffer.variables[i].offset = offset;
		offset += buffer.variables[i].size;
	}
	buffer.size = offset;
}

void ComputeBoundingBox(const Vector<glm::vec4>& points, BoundingBox& box)
{
	Vector<glm::vec3> points3;
	points3.resize(points.size());
	for (uint i = 0; i < points.size(); i++)
		points3[i] = reinterpret_cast<const glm::vec3&>(points[i]);

	BoundingBox::CreateFromPoints(box, points3.size(), points3.data(), sizeof(glm::vec3));
}


int main(int argc, const char** argv)
{
	SetGraphicsAPI(SE_GFX_D3D11);

	GraphicsContext context;
	{
		GraphicsContext::CreateInfo info = {};
		info.debugEnabled = true;
		if (!context.Create(info))
			return -1;
	}

	GraphicsWindow window;
	{
		GraphicsWindow::CreateInfo info = {};
		info.width = 800;
		info.height = 800;
		info.title = "TestApp";
		info.windowStyle = WS_OVERLAPPEDWINDOW;
		if (!window.Create(info))
			return -1;
	}

	Surface surface;
	{
		if (!surface.Create(&window))
			return -1;
	}

	Map<BaseMesh*, BoundingBox> meshBounds;

	BaseMesh cube;
	{
		Vector<glm::vec4> verts = {
			{ -1.0f, 0.0f, +1.0f, +1.0f },
			{ +1.0f, 0.0f, +1.0f, +1.0f },
			{ +1.0f, 2.0f, +1.0f, +1.0f },
			{ -1.0f, 2.0f, +1.0f, +1.0f },
			{ -1.0f, 0.0f, -1.0f, +1.0f },
			{ +1.0f, 0.0f, -1.0f, +1.0f },
			{ +1.0f, 2.0f, -1.0f, +1.0f },
			{ -1.0f, 2.0f, -1.0f, +1.0f },
		};

		Vector<uint> indices = {
			0, 1, 2,
			0, 2, 3,

			1, 5, 6,
			1, 6, 2,

			5, 4, 7,
			5, 7, 6,

			4, 0, 3,
			4, 3, 7,

			3, 2, 6,
			3, 6, 7,

			4, 5, 1,
			4, 1, 0,
		};

		BaseMesh::CreateInfo info = {};
		info.vertexStride = sizeof(glm::vec4);
		info.numVerts = verts.size();
		info.pVerts = verts.data();
		info.numIndices = indices.size();
		info.pIndices = indices.data();

		if (!cube.Create(info))
			return -1;

		ComputeBoundingBox(verts, meshBounds[&cube]);
	}

	BaseMesh plane;
	{
		Vector<glm::vec4> verts = {
			{ -1.0f, 0.0f, +1.0f, 1.0f },
			{ +1.0f, 0.0f, +1.0f, 1.0f },
			{ +1.0f, 0.0f, -1.0f, 1.0f },
			{ -1.0f, 0.0f, -1.0f, 1.0f },
		};

		Vector<uint> indices = {
			0, 1, 2,
			0, 2, 3,
		};

		BaseMesh::CreateInfo info = {};
		info.vertexStride = sizeof(glm::vec4);
		info.numVerts = verts.size();
		info.pVerts = verts.data();
		info.numIndices = indices.size();
		info.pIndices = indices.data();

		if (!plane.Create(info))
			return false;

		ComputeBoundingBox(verts, meshBounds[&plane]);
	}

	Map<ShaderBindingType, ShaderBindingData> bindingMap;
	FillShaderBufferInfo(SBT_CAMERA, bindingMap[SBT_CAMERA].bufferInfo);
	FillShaderBufferInfo(SBT_OBJECT, bindingMap[SBT_OBJECT].bufferInfo);
	FillShaderBufferInfo(SBT_SHADOW, bindingMap[SBT_SHADOW].bufferInfo);

	String shaderPath = "";

	BaseShader coreShader, depthShader;
	{
		BaseShader::CreateInfo info = {};
		info.buffers[bindingMap[SBT_CAMERA].bufferInfo.name] = bindingMap[SBT_CAMERA].bufferInfo;
		info.buffers[bindingMap[SBT_OBJECT].bufferInfo.name] = bindingMap[SBT_OBJECT].bufferInfo;
		info.buffers[bindingMap[SBT_SHADOW].bufferInfo.name] = bindingMap[SBT_SHADOW].bufferInfo;

		IVertexElement vtxElem;
		vtxElem.format = VIF_FLOAT4;
		vtxElem.offset = 0;
		vtxElem.size = sizeof(glm::vec4);
		sprintf_s(vtxElem.semantic, "POSITION");
		info.vertexElements.push_back(vtxElem);

		FileStream fs;

		fs.OpenForRead((shaderPath + "core.vs.cso").c_str());
		fs.ReadBuffer(info.vertexBinaries[SE_GFX_D3D11]);
		fs.Close();

		if (!depthShader.Create(info))
			return -1;

		fs.OpenForRead((shaderPath + "core.ps.cso").c_str());
		fs.ReadBuffer(info.pixelBinaries[SE_GFX_D3D11]);
		fs.Close();

		IShaderResource resource = {};
		resource.bindingCount = 1;
		resource.bindType = SBT_SHADOW;
		resource.stages = SS_PIXEL;

		sprintf_s(resource.name, "DepthTexture");
		resource.dimension = SRD_TEXTURE_2D;
		resource.type = SRT_TEXTURE;
		resource.binding[SE_GFX_D3D11] = 0;
		//resource.binding[SE_GFX_VULKAN] = 0;
		info.resources[resource.name] = resource;

		sprintf_s(resource.name, "Sampler");
		resource.binding[SE_GFX_D3D11] = 0;
		//resource.binding[SE_GFX_VULKAN] = 0;
		resource.type = SRT_SAMPLER;
		info.resources[resource.name] = resource;

		if (!coreShader.Create(info))
			return -1;
	}

	for (auto& binding : bindingMap)
	{
		{
			UniformBuffer::CreateInfo info = {};
			info.isShared = true;
			info.size = binding.second.bufferInfo.size;
			if (!binding.second.ubo.Create(info))
				return -1;
		}

		{
			ShaderBindings::CreateInfo info = {};
			info.pShader = &coreShader;
			info.type = binding.first;
			if (!binding.second.bindings.Create(info))
				return -1;

			if (!binding.second.bindings.SetUniformBuffer(binding.second.bufferInfo.name, &binding.second.ubo))
				return -1;
		}
	}

	GraphicsPipeline corePipeline, depthPipeline;
	{
		GraphicsPipeline::CreateInfo info = {};
#ifdef GLM_FORCE_LEFT_HANDED
		info.settings.rasterizer.frontFace = SE_FF_CLOCKWISE;
#endif

		info.pShader = &coreShader;
		if (!corePipeline.Create(info))
			return -1;

		info.pShader = &depthShader;
		info.settings.rasterizer.slopeScaledDepthBias = 1.0f;
		if (!depthPipeline.Create(info))
			return -1;
	}

	m_CopyOfCascadeConfig.m_nCascadeLevels = 4;
	m_CopyOfCascadeConfig.m_iBufferSize = 2048;

	RenderTarget depthTarget;
	{
		RenderTarget::CreateInfo info = {};
		info.width = m_CopyOfCascadeConfig.m_iBufferSize * m_CopyOfCascadeConfig.m_nCascadeLevels;
		info.height = m_CopyOfCascadeConfig.m_iBufferSize;
		info.hasDepthBuffer = true;

		if (!depthTarget.Create(info))
			return -1;
	}

	Sampler sampler;
	{
		Sampler::CreateInfo info = {};
		info.settings.anisotropicMode = SE_AM_OFF;
		info.settings.filterMode = SE_FM_NEAREST;
		info.settings.wrapMode = SE_WM_CLAMP_TO_EDGE;
		if (!sampler.Create(info))
			return -1;
	}

	if (!bindingMap[SBT_SHADOW].bindings.SetTexture("DepthTexture", depthTarget.GetDepthTexture()))
		return -1;

	if (!bindingMap[SBT_SHADOW].bindings.SetSampler("Sampler", &sampler))
		return -1;

	auto* cmdBuffer = surface.GetCommandBuffer();

	float nearClip = 0.05f;
	float farClip = 300.0f;
	glm::mat4 projMatrix = glm::perspective(glm::radians(45.0f), 1.0f, nearClip, farClip);

	float coordinateSystem = -1.0f;
#ifdef GLM_FORCE_LEFT_HANDED
	coordinateSystem = -coordinateSystem;
#endif

	glm::ivec2 mousePos(0, 0);
	glm::vec4 viewAngles = glm::vec4(618.0f, -19.0f, 0.0f, 0.0f);
	glm::vec4 viewPosition = glm::vec4(-19.f, 3.8f, -12.8f, 1.0f);
	glm::vec4 viewSpeed = glm::vec4(0.1f, 0.1f, 0.1f, 0.1f);

	struct RenderNode
	{
		BaseMesh* pMesh;
		glm::mat4 worldMatrix;
		bool castsShadow;
	};

	FLOAT worldRange = 40.0f;

	Vector<RenderNode> renderNodes;
	uint cubeCount = 30;
	for (uint i = 0; i < cubeCount; i++)
	{
		RenderNode planeNode;
		planeNode.castsShadow = true;
		planeNode.pMesh = &cube;

		glm::vec4 t = glm::vec4(rand() / (float)RAND_MAX, 0.0f, rand() / (float)RAND_MAX, 0.0f);
		glm::vec4 pos = glm::mix(glm::vec4(-worldRange, 0.0f, -worldRange, 0.0f), glm::vec4(worldRange, 0.0f, worldRange, 0.0f), t);
		planeNode.worldMatrix = glm::translate(glm::vec3(pos)) * glm::scale(glm::vec3(1.0f, 1.0f + (20.0f * (rand() / float(RAND_MAX))), 1.0f));
		renderNodes.push_back(planeNode);
	}

	RenderNode planeNode;
	planeNode.castsShadow = false;
	planeNode.pMesh = &plane;
	planeNode.worldMatrix = glm::scale(glm::vec3(worldRange, 1.0f, worldRange));
	renderNodes.push_back(planeNode);

	Vector<glm::mat4> worldMatrices;
	worldMatrices.resize(renderNodes.size());

	IShaderBindingsBindState objectBindState = {};
	objectBindState.DynamicIndices[0].first = bindingMap[SBT_OBJECT].bufferInfo.name;

	IShaderBindingsBindState cameraBindState = {};
	cameraBindState.DynamicIndices[0].first = bindingMap[SBT_CAMERA].bufferInfo.name;

	m_vSceneAABBMin = g_vFLTMAX;
	m_vSceneAABBMax = g_vFLTMIN;
	for (auto& node : renderNodes)
	{
		BoundingBox box;
		meshBounds[node.pMesh].Transform(box, node.worldMatrix);

		glm::vec4 vExtents = box.Extents;
		glm::vec4 vMin = glm::vec4Subtract(box.Center, vExtents);
		glm::vec4 vMax = glm::vec4Add(box.Center, vExtents);

		m_vSceneAABBMin = glm::min(m_vSceneAABBMin, vMin);
		m_vSceneAABBMin = glm::min(m_vSceneAABBMin, vMax);

		m_vSceneAABBMax = glm::max(m_vSceneAABBMax, vMin);
		m_vSceneAABBMax = glm::max(m_vSceneAABBMax, vMax);
	}

	m_iCascadePartitionsZeroToOne[0] = 5;
	m_iCascadePartitionsZeroToOne[1] = 15;
	m_iCascadePartitionsZeroToOne[2] = 60;
	m_iCascadePartitionsZeroToOne[3] = 100;
	m_iCascadePartitionsZeroToOne[4] = 100;
	m_iCascadePartitionsZeroToOne[5] = 100;
	m_iCascadePartitionsZeroToOne[6] = 100;
	m_iCascadePartitionsZeroToOne[7] = 100;

	m_iCascadePartitionsMax = 100;

#if 0
	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	float clipRange = farClip - nearClip;
	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	float cascadeSplitLambda = 0.85f;

	for (int i = 0; i < m_CopyOfCascadeConfig.m_nCascadeLevels; i++) {
		float p = (i + 1) / static_cast<float>(m_CopyOfCascadeConfig.m_nCascadeLevels);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		m_iCascadePartitionsZeroToOne[i] = INT((d - nearClip)/* / clipRange*/);
	}
	m_iCascadePartitionsMax = INT(m_iCascadePartitionsZeroToOne[m_CopyOfCascadeConfig.m_nCascadeLevels - 1]);
#endif


	glm::vec4 lightDir = glm::normalize(glm::vec4(20.0f, 20.0f, -20.0f, 0.0));

	window.Open();
	bool bFocus = true;
	while (window.IsAlive())
	{
		GWEventData* pEvents = 0;
		uint nEvents = 0;
		window.Update(&pEvents, nEvents);

		for (uint i = 0; i < nEvents; i++)
		{
			if (pEvents[i].type == GWE_MOUSE_DOWN && pEvents[i].mouseButtonCode == MOUSE_LEFT)
				window.GetMousePosition(mousePos.x, mousePos.y);
			if (pEvents[i].type == GWE_FOCUS_LOST)
				bFocus = false;
			if (pEvents[i].type == GWE_FOCUS_SET)
				bFocus = true;
		}

		//bFocus = false;
		if (bFocus)
		{
			if (window.KeyDown(KEY_LBUTTON))
			{
				glm::ivec2 currPos;
				window.GetMousePosition(currPos.x, currPos.y);
				glm::vec4 delta = glm::vec4(currPos.x - mousePos.x, currPos.y - mousePos.y, 0.0f, 0.0f);
				viewAngles = glm::vec4Add(viewAngles, glm::vec4Multiply(delta, viewSpeed));
				mousePos = currPos;
			}
		}

		glm::mat4 viewRotation = glm::mat4RotationY(glm::radians(glm::vec4GetX(viewAngles))) * glm::mat4RotationX(glm::radians(glm::vec4GetY(viewAngles)));

		if (bFocus)
		{
			if (window.KeyDown(KEY_A))
				viewPosition = glm::vec4Subtract(viewPosition, viewRotation[0]);
			if (window.KeyDown(KEY_D))
				viewPosition = glm::vec4Add(viewPosition, viewRotation[0]);
			if (window.KeyDown(KEY_W))
				viewPosition = glm::vec4Add(viewPosition, viewRotation[2] * coordinateSystem);
			if (window.KeyDown(KEY_S))
				viewPosition = glm::vec4Subtract(viewPosition, viewRotation[2] * coordinateSystem);


			if (window.KeyDown(KEY_UP))
				lightDir = glm::vec44Transform(lightDir, glm::mat4RotationX(glm::radians(1.0f)));
			if (window.KeyDown(KEY_DOWN))
				lightDir = glm::vec44Transform(lightDir, glm::mat4RotationX(glm::radians(-1.0f)));
			if (window.KeyDown(KEY_RIGHT))
				lightDir = glm::vec44Transform(lightDir, glm::mat4RotationY(glm::radians(1.0f)));
			if (window.KeyDown(KEY_LEFT))
				lightDir = glm::vec44Transform(lightDir, glm::mat4RotationY(glm::radians(-1.0f)));
		}

		glm::mat4 lightMatrix = glm::lookAt(glm::vec3(lightDir), glm::vec3(g_vZero), glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 viewMatrix = glm::translate(glm::vec3(viewPosition)) * viewRotation;
		viewMatrix = glm::inverse(viewMatrix);

		glm::mat4 viewProj = projMatrix * viewMatrix;

		UpdateShadows(nearClip, farClip, projMatrix, viewMatrix, lightMatrix);

		struct CameraBufferData
		{
			glm::mat4 ViewProj;
			glm::mat4 View;
		} cameraMatrices[1 + MAX_CASCADES];

		for (int i = 0; i < m_CopyOfCascadeConfig.m_nCascadeLevels; i++)
		{
			cameraMatrices[i].ViewProj = m_matShadowProj[i] * lightMatrix;
			cameraMatrices[i].View = lightMatrix;
		}

		CameraBufferData mainCamData;
		mainCamData.ViewProj = viewProj;
		mainCamData.View = viewMatrix;
		cameraMatrices[m_CopyOfCascadeConfig.m_nCascadeLevels] = GraphicsWindow::KeyDown(KEY_C) ? cameraMatrices[0] : mainCamData;


		bindingMap[SBT_CAMERA].ubo.UpdateShared(cameraMatrices, 1 + m_CopyOfCascadeConfig.m_nCascadeLevels);

		for (uint i = 0; i < renderNodes.size(); i++)
			worldMatrices[i] = renderNodes[i].worldMatrix;
		bindingMap[SBT_OBJECT].ubo.UpdateShared(worldMatrices.data(), worldMatrices.size());


		struct ShadowBufferData
		{
			glm::mat4 ShadowMatrices[MAX_CASCADES];
			glm::mat4 ShadowSplitDepths;
		} shadowBufferData;

		shadowBufferData.ShadowSplitDepths = glm::mat4(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, coordinateSystem, 1.0f / m_CopyOfCascadeConfig.m_nCascadeLevels, 1.0f / m_CopyOfCascadeConfig.m_iBufferSize);
		for (int i = 0; i < m_CopyOfCascadeConfig.m_nCascadeLevels; i++)
		{
			shadowBufferData.ShadowMatrices[i] = cameraMatrices[i].ViewProj;
			reinterpret_cast<FLOAT*>(&shadowBufferData.ShadowSplitDepths[0])[i] = m_fCascadePartitionsFrustum[i];
		}
		bindingMap[SBT_SHADOW].ubo.Update(&shadowBufferData);

		surface.StartFrame();

		depthTarget.Bind(cmdBuffer);
		depthShader.Bind(cmdBuffer);
		depthPipeline.Bind(cmdBuffer);
		for (int c = 0; c < m_CopyOfCascadeConfig.m_nCascadeLevels; c++)
		{
			float x, y, w, h;
			x = float(c * m_CopyOfCascadeConfig.m_iBufferSize);
			y = float(0.0f);
			w = float(m_CopyOfCascadeConfig.m_iBufferSize);
			h = float(m_CopyOfCascadeConfig.m_iBufferSize);

			cmdBuffer->SetViewport(x, y, w, h);
			cmdBuffer->SetScissor(x, y, w, h);
			cameraBindState.DynamicIndices[0].second = c;
			bindingMap[SBT_CAMERA].bindings.Bind(cmdBuffer, &cameraBindState);

			for (uint i = 0; i < renderNodes.size(); i++)
			{
				auto& node = renderNodes[i];
				if (node.castsShadow)
				{
					node.pMesh->Bind(cmdBuffer);
					objectBindState.DynamicIndices[0].second = i;
					bindingMap[SBT_OBJECT].bindings.Bind(cmdBuffer, &objectBindState);
					cmdBuffer->DrawIndexed(node.pMesh->GetNumIndices(), 1, 0, 0, 0);
					node.pMesh->Unbind(cmdBuffer);
				}
			}
		}
		depthTarget.Unbind(cmdBuffer);

		surface.Bind(cmdBuffer);

		coreShader.Bind(cmdBuffer);
		corePipeline.Bind(cmdBuffer);
		bindingMap[SBT_SHADOW].bindings.Bind(cmdBuffer);

		cameraBindState.DynamicIndices[0].second = m_CopyOfCascadeConfig.m_nCascadeLevels;
		bindingMap[SBT_CAMERA].bindings.Bind(cmdBuffer, &cameraBindState);

		for (uint i = 0; i < renderNodes.size(); i++)
		{
			auto& node = renderNodes[i];
			node.pMesh->Bind(cmdBuffer);
			objectBindState.DynamicIndices[0].second = i;
			bindingMap[SBT_OBJECT].bindings.Bind(cmdBuffer, &objectBindState);
			cmdBuffer->DrawIndexed(node.pMesh->GetNumIndices(), 1, 0, 0, 0);
			node.pMesh->Unbind(cmdBuffer);
		}

		bindingMap[SBT_SHADOW].ubo.Unbind(cmdBuffer);
		corePipeline.Unbind(cmdBuffer);
		coreShader.Unbind(cmdBuffer);

		surface.Unbind(cmdBuffer);
		surface.EndFrame();
	}


	return 0;
}

//--------------------------------------------------------------------------------------
// Computing an accurate near and flar plane will decrease surface acne and Peter-panning.
// Surface acne is the term for erroneous self shadowing.  Peter-panning is the effect where
// shadows disappear near the base of an object.
// As offsets are generally used with PCF filtering due self shadowing issues, computing the
// correct near and far planes becomes even more important.
// This concept is not complicated, but the intersection code is.
//--------------------------------------------------------------------------------------
void ComputeNearAndFar(FLOAT& fNearPlane,
	FLOAT& fFarPlane,
	glm::vec4 vLightCameraOrthographicMin,
	glm::vec4 vLightCameraOrthographicMax,
	glm::vec4* pvPointsInCameraView)
{

	//--------------------------------------------------------------------------------------
	// Used to compute an intersection of the orthographic projection and the Scene AABB
	//--------------------------------------------------------------------------------------
	struct Triangle
	{
		glm::vec4 pt[3];
		bool culled;
	};

	// Initialize the near and far planes
	fNearPlane = FLT_MAX;
	fFarPlane = -FLT_MAX;

	Triangle triangleList[16];
	INT iTriangleCnt = 1;

	triangleList[0].pt[0] = pvPointsInCameraView[0];
	triangleList[0].pt[1] = pvPointsInCameraView[1];
	triangleList[0].pt[2] = pvPointsInCameraView[2];
	triangleList[0].culled = false;

	// These are the indices used to tesselate an AABB into a list of triangles.
	static const INT iAABBTriIndexes[] =
	{
		0,1,2,  1,2,3,
		4,5,6,  5,6,7,
		0,2,4,  2,4,6,
		1,3,5,  3,5,7,
		0,1,4,  1,4,5,
		2,3,6,  3,6,7
	};

	INT iPointPassesCollision[3];

	// At a high level: 
	// 1. Iterate over all 12 triangles of the AABB.  
	// 2. Clip the triangles against each plane. Create new triangles as needed.
	// 3. Find the min and max z values as the near and far plane.

	//This is easier because the triangles are in camera spacing making the collisions tests simple comparisions.

	float fLightCameraOrthographicMinX = glm::vec4GetX(vLightCameraOrthographicMin);
	float fLightCameraOrthographicMaxX = glm::vec4GetX(vLightCameraOrthographicMax);
	float fLightCameraOrthographicMinY = glm::vec4GetY(vLightCameraOrthographicMin);
	float fLightCameraOrthographicMaxY = glm::vec4GetY(vLightCameraOrthographicMax);

	for (INT AABBTriIter = 0; AABBTriIter < 12; ++AABBTriIter)
	{

		triangleList[0].pt[0] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 0]];
		triangleList[0].pt[1] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 1]];
		triangleList[0].pt[2] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 2]];
		iTriangleCnt = 1;
		triangleList[0].culled = FALSE;

		// Clip each invidual triangle against the 4 frustums.  When ever a triangle is clipped into new triangles, 
		//add them to the list.
		for (INT frustumPlaneIter = 0; frustumPlaneIter < 4; ++frustumPlaneIter)
		{

			FLOAT fEdge;
			INT iComponent;

			if (frustumPlaneIter == 0)
			{
				fEdge = fLightCameraOrthographicMinX; // todo make float temp
				iComponent = 0;
			}
			else if (frustumPlaneIter == 1)
			{
				fEdge = fLightCameraOrthographicMaxX;
				iComponent = 0;
			}
			else if (frustumPlaneIter == 2)
			{
				fEdge = fLightCameraOrthographicMinY;
				iComponent = 1;
			}
			else
			{
				fEdge = fLightCameraOrthographicMaxY;
				iComponent = 1;
			}

			for (INT triIter = 0; triIter < iTriangleCnt; ++triIter)
			{
				// We don't delete triangles, so we skip those that have been culled.
				if (!triangleList[triIter].culled)
				{
					INT iInsideVertCount = 0;
					glm::vec4 tempOrder;
					// Test against the correct frustum plane.
					// This could be written more compactly, but it would be harder to understand.

					if (frustumPlaneIter == 0)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (glm::vec4GetX(triangleList[triIter].pt[triPtIter]) >
								glm::vec4GetX(vLightCameraOrthographicMin))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else if (frustumPlaneIter == 1)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (glm::vec4GetX(triangleList[triIter].pt[triPtIter]) <
								glm::vec4GetX(vLightCameraOrthographicMax))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else if (frustumPlaneIter == 2)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (glm::vec4GetY(triangleList[triIter].pt[triPtIter]) >
								glm::vec4GetY(vLightCameraOrthographicMin))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (glm::vec4GetY(triangleList[triIter].pt[triPtIter]) <
								glm::vec4GetY(vLightCameraOrthographicMax))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}

					// Move the points that pass the frustum test to the begining of the array.
					if (iPointPassesCollision[1] && !iPointPassesCollision[0])
					{
						tempOrder = triangleList[triIter].pt[0];
						triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = tempOrder;
						iPointPassesCollision[0] = TRUE;
						iPointPassesCollision[1] = FALSE;
					}
					if (iPointPassesCollision[2] && !iPointPassesCollision[1])
					{
						tempOrder = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = triangleList[triIter].pt[2];
						triangleList[triIter].pt[2] = tempOrder;
						iPointPassesCollision[1] = TRUE;
						iPointPassesCollision[2] = FALSE;
					}
					if (iPointPassesCollision[1] && !iPointPassesCollision[0])
					{
						tempOrder = triangleList[triIter].pt[0];
						triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = tempOrder;
						iPointPassesCollision[0] = TRUE;
						iPointPassesCollision[1] = FALSE;
					}

					if (iInsideVertCount == 0)
					{ // All points failed. We're done,  
						triangleList[triIter].culled = true;
					}
					else if (iInsideVertCount == 1)
					{// One point passed. Clip the triangle against the Frustum plane
						triangleList[triIter].culled = false;

						// 
						glm::vec4 vVert0ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[0];
						glm::vec4 vVert0ToVert2 = triangleList[triIter].pt[2] - triangleList[triIter].pt[0];

						// Find the collision ratio.
						FLOAT fHitPointTimeRatio = fEdge - glm::vec4GetByIndex(triangleList[triIter].pt[0], iComponent);
						// Calculate the distance along the vector as ratio of the hit ratio to the component.
						FLOAT fDistanceAlongVector01 = fHitPointTimeRatio / glm::vec4GetByIndex(vVert0ToVert1, iComponent);
						FLOAT fDistanceAlongVector02 = fHitPointTimeRatio / glm::vec4GetByIndex(vVert0ToVert2, iComponent);
						// Add the point plus a percentage of the vector.
						vVert0ToVert1 *= fDistanceAlongVector01;
						vVert0ToVert1 += triangleList[triIter].pt[0];
						vVert0ToVert2 *= fDistanceAlongVector02;
						vVert0ToVert2 += triangleList[triIter].pt[0];

						triangleList[triIter].pt[1] = vVert0ToVert2;
						triangleList[triIter].pt[2] = vVert0ToVert1;

					}
					else if (iInsideVertCount == 2)
					{ // 2 in  // tesselate into 2 triangles


						// Copy the triangle\(if it exists) after the current triangle out of
						// the way so we can override it with the new triangle we're inserting.
						triangleList[iTriangleCnt] = triangleList[triIter + 1];

						triangleList[triIter].culled = false;
						triangleList[triIter + 1].culled = false;

						// Get the vector from the outside point into the 2 inside points.
						glm::vec4 vVert2ToVert0 = triangleList[triIter].pt[0] - triangleList[triIter].pt[2];
						glm::vec4 vVert2ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[2];

						// Get the hit point ratio.
						FLOAT fHitPointTime_2_0 = fEdge - glm::vec4GetByIndex(triangleList[triIter].pt[2], iComponent);
						FLOAT fDistanceAlongVector_2_0 = fHitPointTime_2_0 / glm::vec4GetByIndex(vVert2ToVert0, iComponent);
						// Calcaulte the new vert by adding the percentage of the vector plus point 2.
						vVert2ToVert0 *= fDistanceAlongVector_2_0;
						vVert2ToVert0 += triangleList[triIter].pt[2];

						// Add a new triangle.
						triangleList[triIter + 1].pt[0] = triangleList[triIter].pt[0];
						triangleList[triIter + 1].pt[1] = triangleList[triIter].pt[1];
						triangleList[triIter + 1].pt[2] = vVert2ToVert0;

						//Get the hit point ratio.
						FLOAT fHitPointTime_2_1 = fEdge - glm::vec4GetByIndex(triangleList[triIter].pt[2], iComponent);
						FLOAT fDistanceAlongVector_2_1 = fHitPointTime_2_1 / glm::vec4GetByIndex(vVert2ToVert1, iComponent);
						vVert2ToVert1 *= fDistanceAlongVector_2_1;
						vVert2ToVert1 += triangleList[triIter].pt[2];
						triangleList[triIter].pt[0] = triangleList[triIter + 1].pt[1];
						triangleList[triIter].pt[1] = triangleList[triIter + 1].pt[2];
						triangleList[triIter].pt[2] = vVert2ToVert1;
						// Cncrement triangle count and skip the triangle we just inserted.
						++iTriangleCnt;
						++triIter;


					}
					else
					{ // all in
						triangleList[triIter].culled = false;

					}
				}// end if !culled loop            
			}
		}
		for (INT index = 0; index < iTriangleCnt; ++index)
		{
			if (!triangleList[index].culled)
			{
				// Set the near and far plan and the min and max z values respectivly.
				for (int vertind = 0; vertind < 3; ++vertind)
				{
					float fTriangleCoordZ = -glm::vec4GetZ(triangleList[index].pt[vertind]);
#ifdef GLM_FORCE_LEFT_HANDED
					fTriangleCoordZ = -fTriangleCoordZ;
#endif
					if (fNearPlane > fTriangleCoordZ)
					{
						fNearPlane = fTriangleCoordZ;
					}
					if (fFarPlane < fTriangleCoordZ)
					{
						fFarPlane = fTriangleCoordZ;
					}
				}
			}
		}
	}

}

//--------------------------------------------------------------------------------------
// This function takes the camera's projection matrix and returns the 8
// points that make up a view frustum.
// The frustum is scaled to fit within the Begin and End interval paramaters.
//--------------------------------------------------------------------------------------
void CreateFrustumPointsFromCascadeInterval(float fCascadeIntervalBegin,
	FLOAT fCascadeIntervalEnd,
	glm::mat4 vProjection,
	glm::vec4* pvCornerPointsWorld)
{

#if 0
	BoundingFrustum vViewFrust(vProjection);
	vViewFrust.Near = fCascadeIntervalBegin;
	vViewFrust.Far = fCascadeIntervalEnd;

	static const glm::uvec4 vGrabY = { 0x00000000,0xFFFFFFFF,0x00000000,0x00000000 };
	static const glm::uvec4 vGrabX = { 0xFFFFFFFF,0x00000000,0x00000000,0x00000000 };

	glm::vec4 vRightTop = { vViewFrust.RightSlope,vViewFrust.TopSlope,1.0f,1.0f };
	glm::vec4 vLeftBottom = { vViewFrust.LeftSlope,vViewFrust.BottomSlope,1.0f,1.0f };
	glm::vec4 vNear = { vViewFrust.Near,vViewFrust.Near,vViewFrust.Near,1.0f };
	glm::vec4 vFar = { vViewFrust.Far,vViewFrust.Far,vViewFrust.Far,1.0f };
	glm::vec4 vRightTopNear = glm::vec4Multiply(vRightTop, vNear);
	glm::vec4 vRightTopFar = glm::vec4Multiply(vRightTop, vFar);
	glm::vec4 vLeftBottomNear = glm::vec4Multiply(vLeftBottom, vNear);
	glm::vec4 vLeftBottomFar = glm::vec4Multiply(vLeftBottom, vFar);

	pvCornerPointsWorld[0] = vRightTopNear;
	pvCornerPointsWorld[1] = glm::vec4Select(vRightTopNear, vLeftBottomNear, vGrabX);
	pvCornerPointsWorld[2] = vLeftBottomNear;
	pvCornerPointsWorld[3] = glm::vec4Select(vRightTopNear, vLeftBottomNear, vGrabY);

	pvCornerPointsWorld[4] = vRightTopFar;
	pvCornerPointsWorld[5] = glm::vec4Select(vRightTopFar, vLeftBottomFar, vGrabX);
	pvCornerPointsWorld[6] = vLeftBottomFar;
	pvCornerPointsWorld[7] = glm::vec4Select(vRightTopFar, vLeftBottomFar, vGrabY);
#else

	float nearZ = -1.0f * 1.0f;
		glm::vec3 frustumCorners[8] = {
		glm::vec3(-1.0f,  1.0f, nearZ),
		glm::vec3(1.0f,  1.0f, nearZ),
		glm::vec3(1.0f, -1.0f, nearZ),
		glm::vec3(-1.0f, -1.0f, nearZ),
		glm::vec3(-1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f),
	};

	// Project frustum corners into world space
	glm::mat4 invCam = glm::inverse(vProjection);
	for (uint32_t i = 0; i < 8; i++) {
		glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
		frustumCorners[i] = invCorner / invCorner.w;
	}

	//This range might not be the EXACT value you would get subtracting far - near due to precision 
	float nearFarRange = glm::abs((frustumCorners[4] - frustumCorners[0]).z);
	fCascadeIntervalBegin /= nearFarRange;
	fCascadeIntervalEnd /= nearFarRange;

	for (uint32_t i = 0; i < 4; i++) {
		glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
		frustumCorners[i + 4] = frustumCorners[i] + (dist * fCascadeIntervalEnd);
		frustumCorners[i] = frustumCorners[i] + (dist * fCascadeIntervalBegin);
	}
	for (uint32_t i = 0; i < 8; i++) {

		pvCornerPointsWorld[i] = glm::vec4(frustumCorners[i], 1.0f);
	}
#endif

}


void UpdateShadows(FLOAT nearClip, FLOAT farClip, const glm::mat4& matViewCameraProjection, const glm::mat4& matViewCameraView, const glm::mat4& matLightCameraView)
{
	glm::mat4 matInverseViewCamera = glm::inverse(matViewCameraView);

	// Convert from min max representation to center extents represnetation.
	// This will make it easier to pull the points out of the transformation.
	BoundingBox bb;
	BoundingBox::CreateFromPoints(bb, m_vSceneAABBMin, m_vSceneAABBMax);

	glm::vec3 tmp[8];
	bb.GetCorners(tmp);

	// Transform the scene AABB to Light space.
	glm::vec4 vSceneAABBPointsLightSpace[8];
	for (int index = 0; index < 8; ++index)
	{
		glm::vec4 v = glm::vec4(tmp[index], 1.0f);
		vSceneAABBPointsLightSpace[index] = glm::vec44Transform(v, matLightCameraView);
	}

	FLOAT fFrustumIntervalBegin, fFrustumIntervalEnd;
	glm::vec4 vLightCameraOrthographicMin;  // light space frustrum aabb 
	glm::vec4 vLightCameraOrthographicMax;
	FLOAT fCameraNearFarRange = farClip - nearClip;

	glm::vec4 vWorldUnitsPerTexel = g_vZero;

	// We loop over the cascades to calculate the orthographic projection for each cascade.
	for (INT iCascadeIndex = 0; iCascadeIndex < m_CopyOfCascadeConfig.m_nCascadeLevels; ++iCascadeIndex)
	{
		// Calculate the interval of the View Frustum that this cascade covers. We measure the interval 
		// the cascade covers as a Min and Max distance along the Z Axis.
		if (m_eSelectedCascadesFit == FIT_TO_CASCADES)
		{
			// Because we want to fit the orthogrpahic projection tightly around the Cascade, we set the Mimiumum cascade 
			// value to the previous Frustum end Interval
			if (iCascadeIndex == 0) fFrustumIntervalBegin = 0.0f;
			else fFrustumIntervalBegin = (FLOAT)m_iCascadePartitionsZeroToOne[iCascadeIndex - 1];
		}
		else
		{
			// In the FIT_TO_SCENE technique the Cascades overlap eachother.  In other words, interval 1 is coverd by
			// cascades 1 to 8, interval 2 is covered by cascades 2 to 8 and so forth.
			fFrustumIntervalBegin = 0.0f;
		}

		// Scale the intervals between 0 and 1. They are now percentages that we can scale with.
		fFrustumIntervalEnd = (FLOAT)m_iCascadePartitionsZeroToOne[iCascadeIndex];
		fFrustumIntervalBegin /= (FLOAT)m_iCascadePartitionsMax;
		fFrustumIntervalEnd /= (FLOAT)m_iCascadePartitionsMax;
		fFrustumIntervalBegin = fFrustumIntervalBegin * fCameraNearFarRange;
		fFrustumIntervalEnd = fFrustumIntervalEnd * fCameraNearFarRange;
		glm::vec4 vFrustumPoints[8];

		// This function takes the began and end intervals along with the projection matrix and returns the 8
		// points that repreresent the cascade Interval
		CreateFrustumPointsFromCascadeInterval(fFrustumIntervalBegin, fFrustumIntervalEnd,
			matViewCameraProjection, vFrustumPoints);

		vLightCameraOrthographicMin = g_vFLTMAX;
		vLightCameraOrthographicMax = g_vFLTMIN;

		glm::vec4 vTempTranslatedCornerPoint;
		// This next section of code calculates the min and max values for the orthographic projection.
		for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
		{
			// Transform the frustum from camera view space to world space.
			vFrustumPoints[icpIndex] = glm::vec44Transform(vFrustumPoints[icpIndex], matInverseViewCamera);
			// Transform the point from world space to Light Camera Space.
			vTempTranslatedCornerPoint = glm::vec44Transform(vFrustumPoints[icpIndex], matLightCameraView);
			// Find the closest point.
			vLightCameraOrthographicMin = glm::min(vTempTranslatedCornerPoint, vLightCameraOrthographicMin);
			vLightCameraOrthographicMax = glm::max(vTempTranslatedCornerPoint, vLightCameraOrthographicMax);
		}

		// This code removes the shimmering effect along the edges of shadows due to
		// the light changing to fit the camera.
		if (m_eSelectedCascadesFit == FIT_TO_SCENE)
		{
			// Fit the ortho projection to the cascades far plane and a near plane of zero. 
			// Pad the projection to be the size of the diagonal of the Frustum partition. 
			// 
			// To do this, we pad the ortho transform so that it is always big enough to cover 
			// the entire camera view frustum.
			glm::vec4 vDiagonal = vFrustumPoints[0] - vFrustumPoints[6];
			vDiagonal = glm::vec43Length(vDiagonal);

			// The bound is the length of the diagonal of the frustum interval.
			FLOAT fCascadeBound = glm::vec4GetX(vDiagonal);

			// The offset calculated will pad the ortho projection so that it is always the same size 
			// and big enough to cover the entire cascade interval.
			glm::vec4 vBoarderOffset = (vDiagonal -
				(vLightCameraOrthographicMax - vLightCameraOrthographicMin))
				* g_vHalfVector;
			// Set the Z and W components to zero.
			vBoarderOffset *= g_vMultiplySetzwToZero;

			// Add the offsets to the projection.
			vLightCameraOrthographicMax += vBoarderOffset;
			vLightCameraOrthographicMin -= vBoarderOffset;

			// The world units per texel are used to snap the shadow the orthographic projection
			// to texel sized increments.  This keeps the edges of the shadows from shimmering.
			FLOAT fWorldUnitsPerTexel = fCascadeBound / (float)m_CopyOfCascadeConfig.m_iBufferSize;
			vWorldUnitsPerTexel = glm::vec4(fWorldUnitsPerTexel, fWorldUnitsPerTexel, 0.0f, 0.0f);


		}
		else if (m_eSelectedCascadesFit == FIT_TO_CASCADES)
		{

			// We calculate a looser bound based on the size of the PCF blur.  This ensures us that we're 
			// sampling within the correct map.
			float fScaleDuetoBlureAMT = ((float)(m_iPCFBlurSize * 2 + 1)
				/ (float)m_CopyOfCascadeConfig.m_iBufferSize);
			glm::vec4 vScaleDuetoBlureAMT = { fScaleDuetoBlureAMT, fScaleDuetoBlureAMT, 0.0f, 0.0f };


			float fNormalizeByBufferSize = (1.0f / (float)m_CopyOfCascadeConfig.m_iBufferSize);
			glm::vec4 vNormalizeByBufferSize = glm::vec4(fNormalizeByBufferSize, fNormalizeByBufferSize, 0.0f, 0.0f);

			// We calculate the offsets as a percentage of the bound.
			glm::vec4 vBoarderOffset = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
			vBoarderOffset *= g_vHalfVector;
			vBoarderOffset *= vScaleDuetoBlureAMT;
			vLightCameraOrthographicMax += vBoarderOffset;
			vLightCameraOrthographicMin -= vBoarderOffset;

			// The world units per texel are used to snap  the orthographic projection
			// to texel sized increments.  
			// Because we're fitting tighly to the cascades, the shimmering shadow edges will still be present when the 
			// camera rotates.  However, when zooming in or strafing the shadow edge will not shimmer.
			vWorldUnitsPerTexel = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
			vWorldUnitsPerTexel *= vNormalizeByBufferSize;

		}
		float fLightCameraOrthographicMinZ = glm::vec4GetZ(vLightCameraOrthographicMin);


		//if (m_bMoveLightTexelSize)
		{

			// We snape the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
			// This is a matter of integer dividing by the world space size of a texel
			vLightCameraOrthographicMin /= vWorldUnitsPerTexel;
			vLightCameraOrthographicMin = glm::floor(vLightCameraOrthographicMin);
			vLightCameraOrthographicMin *= vWorldUnitsPerTexel;

			vLightCameraOrthographicMax /= vWorldUnitsPerTexel;
			vLightCameraOrthographicMax = glm::floor(vLightCameraOrthographicMax);
			vLightCameraOrthographicMax *= vWorldUnitsPerTexel;

		}

		//These are the unconfigured near and far plane values.  They are purposly awful to show 
		// how important calculating accurate near and far planes is.
		FLOAT fNearPlane = 0.0f;
		FLOAT fFarPlane = 10000.0f;

		if (m_eSelectedNearFarFit == FIT_NEARFAR_AABB)
		{

			glm::vec4 vLightSpaceSceneAABBminValue = g_vFLTMAX;  // world space scene aabb 
			glm::vec4 vLightSpaceSceneAABBmaxValue = g_vFLTMIN;
			// We calculate the min and max vectors of the scene in light space. The min and max "Z" values of the  
			// light space AABB can be used for the near and far plane. This is easier than intersecting the scene with the AABB
			// and in some cases provides similar results.
			for (int index = 0; index < 8; ++index)
			{
				vLightSpaceSceneAABBminValue = glm::min(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBminValue);
				vLightSpaceSceneAABBmaxValue = glm::max(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBmaxValue);
			}

			// The min and max z values are the near and far planes.
			fNearPlane = glm::vec4GetZ(vLightSpaceSceneAABBminValue);
			fFarPlane = glm::vec4GetZ(vLightSpaceSceneAABBmaxValue);
		}
		else if (m_eSelectedNearFarFit == FIT_NEARFAR_SCENE_AABB
			|| m_eSelectedNearFarFit == FIT_NEARFAR_PANCAKING)
		{
			// By intersecting the light frustum with the scene AABB we can get a tighter bound on the near and far plane.
			ComputeNearAndFar(fNearPlane, fFarPlane, vLightCameraOrthographicMin,
				vLightCameraOrthographicMax, vSceneAABBPointsLightSpace);
			if (m_eSelectedNearFarFit == FIT_NEARFAR_PANCAKING)
			{
				if (fLightCameraOrthographicMinZ > fNearPlane)
				{
					fNearPlane = fLightCameraOrthographicMinZ;
				}
			}
		}
		// Create the orthographic projection for this cascade.
		m_matShadowProj[iCascadeIndex] = glm::ortho(glm::vec4GetX(vLightCameraOrthographicMin), glm::vec4GetX(vLightCameraOrthographicMax),
			glm::vec4GetY(vLightCameraOrthographicMin), glm::vec4GetY(vLightCameraOrthographicMax),
			fNearPlane, fFarPlane);
		m_fCascadePartitionsFrustum[iCascadeIndex] = fFrustumIntervalEnd;
	}
}

#else
#include <DirectXMath.h>
#include <DirectXCollision.h>

using namespace SunEngine;
using namespace DirectX;

struct ShaderBindingData
{
	UniformBuffer ubo;
	ShaderBindings bindings;
	IShaderBuffer bufferInfo;
};

static const XMVECTORF32 g_vFLTMAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
static const XMVECTORF32 g_vFLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
static const XMVECTORF32 g_vHalfVector = { 0.5f, 0.5f, 0.5f, 0.5f };
static const XMVECTORF32 g_vMultiplySetzwToZero = { 1.0f, 1.0f, 0.0f, 0.0f };
static const XMVECTORF32 g_vZero = { 0.0f, 0.0f, 0.0f, 0.0f };

#define MAX_CASCADES 8

XMVECTOR m_vSceneAABBMin, m_vSceneAABBMax;
INT m_iCascadePartitionsZeroToOne[MAX_CASCADES];
INT m_iCascadePartitionsMax;
XMMATRIX m_matShadowProj[MAX_CASCADES];
FLOAT m_fCascadePartitionsFrustum[MAX_CASCADES];

struct
{
	INT m_nCascadeLevels;
	INT m_iBufferSize;
} m_CopyOfCascadeConfig;

void UpdateShadows(FLOAT nearClip, FLOAT farClip, const XMMATRIX& matViewCameraProjection, const XMMATRIX& matViewCameraView, const XMMATRIX& matLightCameraView);

void FillShaderBufferInfo(ShaderBindingType type, IShaderBuffer& buffer)
{
	buffer = {};
	buffer.bindType = type;

	struct ShaderVarDef
	{
		ShaderVarDef()
		{
			type = SDT_UNDEFINED;
			numElements = 1;
		}

		ShaderVarDef(const char* name, ShaderDataType type) : ShaderVarDef()
		{
			this->name = name;
			this->type = type;
		}

		ShaderVarDef(const char* name, ShaderDataType type, uint numElements) : ShaderVarDef(name, type)
		{
			this->numElements = numElements;
		}

		String name;
		ShaderDataType type;
		uint numElements;
	};

	Vector<ShaderVarDef> variables;

	String bufferName;
	if (type == SBT_CAMERA)
	{
		bufferName = "CameraBuffer";
		buffer.binding[SE_GFX_D3D11] = 0;
		buffer.binding[SE_GFX_VULKAN] = 0;
		buffer.stages = SS_VERTEX;

		variables =
		{
			ShaderVarDef("ViewProjMatrix", SDT_MAT4),
			ShaderVarDef("ViewMatrix", SDT_MAT4)
		};
	}
	else if (type == SBT_OBJECT)
	{
		bufferName = "ObjectBuffer";
		buffer.binding[SE_GFX_D3D11] = 1;
		buffer.binding[SE_GFX_VULKAN] = 0;
		buffer.stages = SS_VERTEX;

		variables =
		{
			ShaderVarDef("WorldMatirx", SDT_MAT4)
		};
	}
	else if (type == SBT_SHADOW)
	{
		bufferName = "ShadowBuffer";
		buffer.binding[SE_GFX_D3D11] = 2;
		buffer.binding[SE_GFX_VULKAN] = 0;
		buffer.stages = SS_VERTEX | SS_PIXEL;

		variables =
		{
			ShaderVarDef("ShadowMatrices", SDT_MAT4, MAX_CASCADES),
			ShaderVarDef("ShadowSplitDepths", SDT_MAT4)
		};
	}

	sprintf_s(buffer.name, bufferName.c_str());
	buffer.numVariables = variables.size();
	uint offset = 0;
	for (uint i = 0; i < variables.size(); i++)
	{
		sprintf_s(buffer.variables[i].name, variables[i].name.c_str());
		buffer.variables[i].type = variables[i].type;
		switch (variables[i].type)
		{
		case SDT_FLOAT: buffer.variables[i].size = sizeof(float); break;
		case SDT_FLOAT2: buffer.variables[i].size = sizeof(XMFLOAT2); break;
		case SDT_FLOAT3: buffer.variables[i].size = sizeof(XMFLOAT3); break;
		case SDT_FLOAT4: buffer.variables[i].size = sizeof(XMFLOAT4); break;
		case SDT_MAT2: buffer.variables[i].size = sizeof(XMFLOAT2) * 2; break;
		case SDT_MAT3: buffer.variables[i].size = sizeof(XMFLOAT3) * 3; break;
		case SDT_MAT4: buffer.variables[i].size = sizeof(XMFLOAT4) * 4; break;
		default:
			break;
		}

		buffer.variables[i].size *= variables[i].numElements;
		buffer.variables[i].numElements = variables[i].numElements;
		buffer.variables[i].offset = offset;
		offset += buffer.variables[i].size;
	}
	buffer.size = offset;
}

void ComputeBoundingBox(const Vector<XMFLOAT4>& points, BoundingBox& box)
{
	Vector<XMFLOAT3> points3;
	points3.resize(points.size());
	for (uint i = 0; i < points.size(); i++)
		points3[i] = reinterpret_cast<const XMFLOAT3&>(points[i]);

	BoundingBox::CreateFromPoints(box, points3.size(), points3.data(), sizeof(XMFLOAT3));
}


int main(int argc, const char** argv)
{
	SetGraphicsAPI(SE_GFX_D3D11);

	GraphicsContext context;
	{
		GraphicsContext::CreateInfo info = {};
		info.debugEnabled = true;
		if (!context.Create(info))
			return -1;
	}

	GraphicsWindow window;
	{
		GraphicsWindow::CreateInfo info = {};
		info.width = 800;
		info.height = 800;
		info.title = "TestApp";
		info.windowStyle = WS_OVERLAPPEDWINDOW;
		if (!window.Create(info))
			return -1;
	}

	Surface surface;
	{
		if (!surface.Create(&window))
			return -1;
	}

	Map<BaseMesh*, BoundingBox> meshBounds;

	BaseMesh cube;
	{
		Vector<XMFLOAT4> verts = {
			{ -1.0f, 0.0f, +1.0f, +1.0f },
			{ +1.0f, 0.0f, +1.0f, +1.0f },
			{ +1.0f, 2.0f, +1.0f, +1.0f },
			{ -1.0f, 2.0f, +1.0f, +1.0f },
			{ -1.0f, 0.0f, -1.0f, +1.0f },
			{ +1.0f, 0.0f, -1.0f, +1.0f },
			{ +1.0f, 2.0f, -1.0f, +1.0f },
			{ -1.0f, 2.0f, -1.0f, +1.0f },
		};

		Vector<uint> indices = {
			0, 1, 2,
			0, 2, 3,

			1, 5, 6,
			1, 6, 2,

			5, 4, 7,
			5, 7, 6,

			4, 0, 3,
			4, 3, 7,

			3, 2, 6,
			3, 6, 7,

			4, 5, 1,
			4, 1, 0,
		};

		BaseMesh::CreateInfo info = {};
		info.vertexStride = sizeof(XMFLOAT4);
		info.numVerts = verts.size();
		info.pVerts = verts.data();
		info.numIndices = indices.size();
		info.pIndices = indices.data();

		if (!cube.Create(info))
			return -1;

		ComputeBoundingBox(verts, meshBounds[&cube]);
	}

	BaseMesh plane;
	{
		Vector<XMFLOAT4> verts = {
			{ -1.0f, 0.0f, +1.0f, 1.0f },
			{ +1.0f, 0.0f, +1.0f, 1.0f },
			{ +1.0f, 0.0f, -1.0f, 1.0f },
			{ -1.0f, 0.0f, -1.0f, 1.0f },
		};

		Vector<uint> indices = {
			0, 1, 2,
			0, 2, 3,
		};

		BaseMesh::CreateInfo info = {};
		info.vertexStride = sizeof(XMFLOAT4);
		info.numVerts = verts.size();
		info.pVerts = verts.data();
		info.numIndices = indices.size();
		info.pIndices = indices.data();

		if (!plane.Create(info))
			return false;

		ComputeBoundingBox(verts, meshBounds[&plane]);
	}

	Map<ShaderBindingType, ShaderBindingData> bindingMap;
	FillShaderBufferInfo(SBT_CAMERA, bindingMap[SBT_CAMERA].bufferInfo);
	FillShaderBufferInfo(SBT_OBJECT, bindingMap[SBT_OBJECT].bufferInfo);
	FillShaderBufferInfo(SBT_SHADOW, bindingMap[SBT_SHADOW].bufferInfo);

	String shaderPath = "";

	BaseShader coreShader, depthShader;
	{
		BaseShader::CreateInfo info = {};
		info.buffers[bindingMap[SBT_CAMERA].bufferInfo.name] = bindingMap[SBT_CAMERA].bufferInfo;
		info.buffers[bindingMap[SBT_OBJECT].bufferInfo.name] = bindingMap[SBT_OBJECT].bufferInfo;
		info.buffers[bindingMap[SBT_SHADOW].bufferInfo.name] = bindingMap[SBT_SHADOW].bufferInfo;

		IVertexElement vtxElem;
		vtxElem.format = VIF_FLOAT4;
		vtxElem.offset = 0;
		vtxElem.size = sizeof(XMFLOAT4);
		sprintf_s(vtxElem.semantic, "POSITION");
		info.vertexElements.push_back(vtxElem);

		FileStream fs;

		fs.OpenForRead((shaderPath + "core.vs.cso").c_str());
		fs.ReadBuffer(info.vertexBinaries[SE_GFX_D3D11]);
		fs.Close();

		if (!depthShader.Create(info))
			return -1;

		fs.OpenForRead((shaderPath + "core.ps.cso").c_str());
		fs.ReadBuffer(info.pixelBinaries[SE_GFX_D3D11]);
		fs.Close();

		IShaderResource resource = {};
		resource.bindingCount = 1;
		resource.bindType = SBT_SHADOW;
		resource.stages = SS_PIXEL;

		sprintf_s(resource.name, "DepthTexture");
		resource.dimension = SRD_TEXTURE_2D;
		resource.type = SRT_TEXTURE;
		resource.binding[SE_GFX_D3D11] = 0;
		//resource.binding[SE_GFX_VULKAN] = 0;
		info.resources[resource.name] = resource;

		sprintf_s(resource.name, "Sampler");
		resource.binding[SE_GFX_D3D11] = 0;
		//resource.binding[SE_GFX_VULKAN] = 0;
		resource.type = SRT_SAMPLER;
		info.resources[resource.name] = resource;

		if (!coreShader.Create(info))
			return -1;
	}

	for (auto& binding : bindingMap)
	{
		{
			UniformBuffer::CreateInfo info = {};
			info.isShared = true;
			info.size = binding.second.bufferInfo.size;
			if (!binding.second.ubo.Create(info))
				return -1;
		}

		{
			ShaderBindings::CreateInfo info = {};
			info.pShader = &coreShader;
			info.type = binding.first;
			if (!binding.second.bindings.Create(info))
				return -1;

			if (!binding.second.bindings.SetUniformBuffer(binding.second.bufferInfo.name, &binding.second.ubo))
				return -1;
		}
	}

	GraphicsPipeline corePipeline, depthPipeline;
	{
		GraphicsPipeline::CreateInfo info = {};
		info.settings.rasterizer.frontFace = SE_FF_CLOCKWISE;

		info.pShader = &coreShader;
		if (!corePipeline.Create(info))
			return -1;

		info.pShader = &depthShader;
		info.settings.rasterizer.slopeScaledDepthBias = 1.0f;
		if (!depthPipeline.Create(info))
			return -1;
	}

	m_CopyOfCascadeConfig.m_nCascadeLevels = 4;
	m_CopyOfCascadeConfig.m_iBufferSize = 2048;

	RenderTarget depthTarget;
	{
		RenderTarget::CreateInfo info = {};
		info.width = m_CopyOfCascadeConfig.m_iBufferSize * m_CopyOfCascadeConfig.m_nCascadeLevels;
		info.height = m_CopyOfCascadeConfig.m_iBufferSize;
		info.hasDepthBuffer = true;

		if (!depthTarget.Create(info))
			return -1;
	}

	Sampler sampler;
	{
		Sampler::CreateInfo info = {};
		info.settings.anisotropicMode = SE_AM_OFF;
		info.settings.filterMode = SE_FM_NEAREST;
		info.settings.wrapMode = SE_WM_CLAMP_TO_EDGE;
		if (!sampler.Create(info))
			return -1;
	}

	if (!bindingMap[SBT_SHADOW].bindings.SetTexture("DepthTexture", depthTarget.GetDepthTexture()))
		return -1;

	if (!bindingMap[SBT_SHADOW].bindings.SetSampler("Sampler", &sampler))
		return -1;

	auto* cmdBuffer = surface.GetCommandBuffer();

	float nearClip = 0.05f;
	float farClip = 300.0f;
	XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), 1.0f, nearClip, farClip);

	XMINT2 mousePos(0, 0);
	XMVECTOR viewAngles = XMVectorSet(461, 5.0f, 0.0f, 0.0f);
	XMVECTOR viewPosition = XMVectorSet(-29.f, 4.8f, -12.8f, 1.0f);
	XMVECTOR viewSpeed = XMVectorSet(0.1f, 0.1f, 0.1f, 0.1f);

	struct RenderNode
	{
		BaseMesh* pMesh;
		XMMATRIX worldMatrix;
		bool castsShadow;
	};

	FLOAT worldRange = 40.0f;

	Vector<RenderNode> renderNodes;
	uint cubeCount = 30;
	for (uint i = 0; i < cubeCount; i++)
	{
		RenderNode planeNode;
		planeNode.castsShadow = true;
		planeNode.pMesh = &cube;

		XMVECTOR t = XMVectorSet(rand() / (float)RAND_MAX, 0.0f, rand() / (float)RAND_MAX, 0.0f);
		XMVECTOR pos = XMVectorLerpV(XMVectorSet(-worldRange, 0.0f, -worldRange, 0.0f), XMVectorSet(worldRange, 0.0f, worldRange, 0.0f), t);
		planeNode.worldMatrix = XMMatrixScaling(1.0f, 1.0f + (20.0f * (rand() / float(RAND_MAX))), 1.0f) * XMMatrixTranslationFromVector(pos);
		renderNodes.push_back(planeNode);
	}

	RenderNode planeNode;
	planeNode.castsShadow = false;
	planeNode.pMesh = &plane;
	planeNode.worldMatrix = XMMatrixScaling(worldRange, 1.0f, worldRange);
	renderNodes.push_back(planeNode);

	Vector<XMMATRIX> worldMatrices;
	worldMatrices.resize(renderNodes.size());

	IShaderBindingsBindState objectBindState = {};
	objectBindState.DynamicIndices[0].first = bindingMap[SBT_OBJECT].bufferInfo.name;

	IShaderBindingsBindState cameraBindState = {};
	cameraBindState.DynamicIndices[0].first = bindingMap[SBT_CAMERA].bufferInfo.name;

	m_vSceneAABBMin = g_FltMax;
	m_vSceneAABBMax = g_FltMin;
	for (auto& node : renderNodes)
	{
		BoundingBox box;
		meshBounds[node.pMesh].Transform(box, node.worldMatrix);

		XMVECTOR vExtents = XMLoadFloat3(&box.Extents);
		XMVECTOR vMin = XMVectorSubtract(XMLoadFloat3(&box.Center), vExtents);
		XMVECTOR vMax = XMVectorAdd(XMLoadFloat3(&box.Center), vExtents);

		m_vSceneAABBMin = XMVectorMin(m_vSceneAABBMin, vMin);
		m_vSceneAABBMin = XMVectorMin(m_vSceneAABBMin, vMax);

		m_vSceneAABBMax = XMVectorMax(m_vSceneAABBMax, vMin);
		m_vSceneAABBMax = XMVectorMax(m_vSceneAABBMax, vMax);
	}

	m_iCascadePartitionsZeroToOne[0] = 5;
	m_iCascadePartitionsZeroToOne[1] = 15;
	m_iCascadePartitionsZeroToOne[2] = 60;
	m_iCascadePartitionsZeroToOne[3] = 100;
	m_iCascadePartitionsZeroToOne[4] = 100;
	m_iCascadePartitionsZeroToOne[5] = 100;
	m_iCascadePartitionsZeroToOne[6] = 100;
	m_iCascadePartitionsZeroToOne[7] = 100;

	m_iCascadePartitionsMax = 100;

#if 0
	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	float clipRange = farClip - nearClip;
	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	float cascadeSplitLambda = 0.85f;

	for (int i = 0; i < m_CopyOfCascadeConfig.m_nCascadeLevels; i++) {
		float p = (i + 1) / static_cast<float>(m_CopyOfCascadeConfig.m_nCascadeLevels);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		m_iCascadePartitionsZeroToOne[i] = INT((d - nearClip)/* / clipRange*/);
	}
	m_iCascadePartitionsMax = INT(m_iCascadePartitionsZeroToOne[m_CopyOfCascadeConfig.m_nCascadeLevels - 1]);
#endif


	XMVECTOR lightDir = XMVector4Normalize(XMVectorSet(20.0f, 20.0f, -20.0f, 0.0));

	window.Open();
	while (window.IsAlive())
	{
		GWEventData* pEvents = 0;
		uint nEvents = 0;
		window.Update(&pEvents, nEvents);

		for (uint i = 0; i < nEvents; i++)
		{
			if (pEvents[i].type == GWE_MOUSE_DOWN && pEvents[i].mouseButtonCode == MOUSE_LEFT)
				window.GetMousePosition(mousePos.x, mousePos.y);
		}

		if (window.KeyDown(KEY_LBUTTON))
		{
			XMINT2 currPos;
			window.GetMousePosition(currPos.x, currPos.y);
			XMVECTOR delta = XMVectorSet(currPos.x - mousePos.x, currPos.y - mousePos.y, 0.0f, 0.0f);
			viewAngles = XMVectorAdd(viewAngles, XMVectorMultiply(delta, viewSpeed));
			mousePos = currPos;
		}

		XMMATRIX viewRotation = XMMatrixRotationX(XMConvertToRadians(XMVectorGetY(viewAngles))) * XMMatrixRotationY(XMConvertToRadians(XMVectorGetX(viewAngles)));

		if (window.KeyDown(KEY_A))
			viewPosition = XMVectorSubtract(viewPosition, viewRotation.r[0]);
		if (window.KeyDown(KEY_D))
			viewPosition = XMVectorAdd(viewPosition, viewRotation.r[0]);
		if (window.KeyDown(KEY_W))
			viewPosition = XMVectorAdd(viewPosition, viewRotation.r[2]);
		if (window.KeyDown(KEY_S))
			viewPosition = XMVectorSubtract(viewPosition, viewRotation.r[2]);


		if (window.KeyDown(KEY_UP))
			lightDir = XMVector4Transform(lightDir, XMMatrixRotationX(XMConvertToRadians(1.0f)));
		if (window.KeyDown(KEY_DOWN))
			lightDir = XMVector4Transform(lightDir, XMMatrixRotationX(XMConvertToRadians(-1.0f)));
		if (window.KeyDown(KEY_RIGHT))
			lightDir = XMVector4Transform(lightDir, XMMatrixRotationY(XMConvertToRadians(1.0f)));
		if (window.KeyDown(KEY_LEFT))
			lightDir = XMVector4Transform(lightDir, XMMatrixRotationY(XMConvertToRadians(-1.0f)));

		XMMATRIX lightMatrix = XMMatrixLookAtLH(lightDir, g_vZero, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

		XMMATRIX viewMatrix = viewRotation * XMMatrixTranslationFromVector(viewPosition);
		XMVECTOR det;
		viewMatrix = XMMatrixInverse(&det, viewMatrix);

		XMMATRIX viewProj = viewMatrix * projMatrix;

		UpdateShadows(nearClip, farClip, projMatrix, viewMatrix, lightMatrix);

		struct CameraBufferData
		{
			XMMATRIX ViewProj;
			XMMATRIX View;
		} cameraMatrices[1 + MAX_CASCADES];

		for (int i = 0; i < m_CopyOfCascadeConfig.m_nCascadeLevels; i++)
		{
			cameraMatrices[i].ViewProj = lightMatrix * m_matShadowProj[i];
			cameraMatrices[i].View = lightMatrix;
		}

		CameraBufferData mainCamData;
		mainCamData.ViewProj = viewProj;
		mainCamData.View = viewMatrix;
		cameraMatrices[m_CopyOfCascadeConfig.m_nCascadeLevels] = GraphicsWindow::KeyDown(KEY_C) ? cameraMatrices[0] : mainCamData;


		bindingMap[SBT_CAMERA].ubo.UpdateShared(cameraMatrices, 1 + m_CopyOfCascadeConfig.m_nCascadeLevels);

		for (uint i = 0; i < renderNodes.size(); i++)
			worldMatrices[i] = renderNodes[i].worldMatrix;
		bindingMap[SBT_OBJECT].ubo.UpdateShared(worldMatrices.data(), worldMatrices.size());


		struct ShadowBufferData
		{
			XMMATRIX ShadowMatrices[MAX_CASCADES];
			XMMATRIX ShadowSplitDepths;
		} shadowBufferData;

		shadowBufferData.ShadowSplitDepths = XMMatrixSet(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f / m_CopyOfCascadeConfig.m_nCascadeLevels, 1.0f / m_CopyOfCascadeConfig.m_iBufferSize);
		for (int i = 0; i < m_CopyOfCascadeConfig.m_nCascadeLevels; i++)
		{
			shadowBufferData.ShadowMatrices[i] = cameraMatrices[i].ViewProj;
			reinterpret_cast<FLOAT*>(shadowBufferData.ShadowSplitDepths.r)[i] = m_fCascadePartitionsFrustum[i];
		}
		bindingMap[SBT_SHADOW].ubo.Update(&shadowBufferData);

		surface.StartFrame();

		depthTarget.Bind(cmdBuffer);
		depthShader.Bind(cmdBuffer);
		depthPipeline.Bind(cmdBuffer);
		for (int c = 0; c < m_CopyOfCascadeConfig.m_nCascadeLevels; c++)
		{
			float x, y, w, h;
			x = float(c * m_CopyOfCascadeConfig.m_iBufferSize);
			y = float(0.0f);
			w = float(m_CopyOfCascadeConfig.m_iBufferSize);
			h = float(m_CopyOfCascadeConfig.m_iBufferSize);

			cmdBuffer->SetViewport(x, y, w, h);
			cmdBuffer->SetScissor(x, y, w, h);
			cameraBindState.DynamicIndices[0].second = c;
			bindingMap[SBT_CAMERA].bindings.Bind(cmdBuffer, &cameraBindState);

			for (uint i = 0; i < renderNodes.size(); i++)
			{
				auto& node = renderNodes[i];
				if (node.castsShadow)
				{
					node.pMesh->Bind(cmdBuffer);
					objectBindState.DynamicIndices[0].second = i;
					bindingMap[SBT_OBJECT].bindings.Bind(cmdBuffer, &objectBindState);
					cmdBuffer->DrawIndexed(node.pMesh->GetNumIndices(), 1, 0, 0, 0);
					node.pMesh->Unbind(cmdBuffer);
				}
			}
		}
		depthTarget.Unbind(cmdBuffer);

		surface.Bind(cmdBuffer);

		coreShader.Bind(cmdBuffer);
		corePipeline.Bind(cmdBuffer);
		bindingMap[SBT_SHADOW].bindings.Bind(cmdBuffer);

		cameraBindState.DynamicIndices[0].second = m_CopyOfCascadeConfig.m_nCascadeLevels;
		bindingMap[SBT_CAMERA].bindings.Bind(cmdBuffer, &cameraBindState);

		for (uint i = 0; i < renderNodes.size(); i++)
		{
			auto& node = renderNodes[i];
			node.pMesh->Bind(cmdBuffer);
			objectBindState.DynamicIndices[0].second = i;
			bindingMap[SBT_OBJECT].bindings.Bind(cmdBuffer, &objectBindState);
			cmdBuffer->DrawIndexed(node.pMesh->GetNumIndices(), 1, 0, 0, 0);
			node.pMesh->Unbind(cmdBuffer);
		}

		bindingMap[SBT_SHADOW].ubo.Unbind(cmdBuffer);
		corePipeline.Unbind(cmdBuffer);
		coreShader.Unbind(cmdBuffer);

		surface.Unbind(cmdBuffer);
		surface.EndFrame();
	}


	return 0;
}

//--------------------------------------------------------------------------------------
// Computing an accurate near and flar plane will decrease surface acne and Peter-panning.
// Surface acne is the term for erroneous self shadowing.  Peter-panning is the effect where
// shadows disappear near the base of an object.
// As offsets are generally used with PCF filtering due self shadowing issues, computing the
// correct near and far planes becomes even more important.
// This concept is not complicated, but the intersection code is.
//--------------------------------------------------------------------------------------
void ComputeNearAndFar(FLOAT& fNearPlane,
	FLOAT& fFarPlane,
	FXMVECTOR vLightCameraOrthographicMin,
	FXMVECTOR vLightCameraOrthographicMax,
	XMVECTOR* pvPointsInCameraView)
{

	//--------------------------------------------------------------------------------------
	// Used to compute an intersection of the orthographic projection and the Scene AABB
	//--------------------------------------------------------------------------------------
	struct Triangle
	{
		XMVECTOR pt[3];
		bool culled;
	};

	// Initialize the near and far planes
	fNearPlane = FLT_MAX;
	fFarPlane = -FLT_MAX;

	Triangle triangleList[16];
	INT iTriangleCnt = 1;

	triangleList[0].pt[0] = pvPointsInCameraView[0];
	triangleList[0].pt[1] = pvPointsInCameraView[1];
	triangleList[0].pt[2] = pvPointsInCameraView[2];
	triangleList[0].culled = false;

	// These are the indices used to tesselate an AABB into a list of triangles.
	static const INT iAABBTriIndexes[] =
	{
		0,1,2,  1,2,3,
		4,5,6,  5,6,7,
		0,2,4,  2,4,6,
		1,3,5,  3,5,7,
		0,1,4,  1,4,5,
		2,3,6,  3,6,7
	};

	INT iPointPassesCollision[3];

	// At a high level: 
	// 1. Iterate over all 12 triangles of the AABB.  
	// 2. Clip the triangles against each plane. Create new triangles as needed.
	// 3. Find the min and max z values as the near and far plane.

	//This is easier because the triangles are in camera spacing making the collisions tests simple comparisions.

	float fLightCameraOrthographicMinX = XMVectorGetX(vLightCameraOrthographicMin);
	float fLightCameraOrthographicMaxX = XMVectorGetX(vLightCameraOrthographicMax);
	float fLightCameraOrthographicMinY = XMVectorGetY(vLightCameraOrthographicMin);
	float fLightCameraOrthographicMaxY = XMVectorGetY(vLightCameraOrthographicMax);

	for (INT AABBTriIter = 0; AABBTriIter < 12; ++AABBTriIter)
	{

		triangleList[0].pt[0] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 0]];
		triangleList[0].pt[1] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 1]];
		triangleList[0].pt[2] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 2]];
		iTriangleCnt = 1;
		triangleList[0].culled = FALSE;

		// Clip each invidual triangle against the 4 frustums.  When ever a triangle is clipped into new triangles, 
		//add them to the list.
		for (INT frustumPlaneIter = 0; frustumPlaneIter < 4; ++frustumPlaneIter)
		{

			FLOAT fEdge;
			INT iComponent;

			if (frustumPlaneIter == 0)
			{
				fEdge = fLightCameraOrthographicMinX; // todo make float temp
				iComponent = 0;
			}
			else if (frustumPlaneIter == 1)
			{
				fEdge = fLightCameraOrthographicMaxX;
				iComponent = 0;
			}
			else if (frustumPlaneIter == 2)
			{
				fEdge = fLightCameraOrthographicMinY;
				iComponent = 1;
			}
			else
			{
				fEdge = fLightCameraOrthographicMaxY;
				iComponent = 1;
			}

			for (INT triIter = 0; triIter < iTriangleCnt; ++triIter)
			{
				// We don't delete triangles, so we skip those that have been culled.
				if (!triangleList[triIter].culled)
				{
					INT iInsideVertCount = 0;
					XMVECTOR tempOrder;
					// Test against the correct frustum plane.
					// This could be written more compactly, but it would be harder to understand.

					if (frustumPlaneIter == 0)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (XMVectorGetX(triangleList[triIter].pt[triPtIter]) >
								XMVectorGetX(vLightCameraOrthographicMin))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else if (frustumPlaneIter == 1)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (XMVectorGetX(triangleList[triIter].pt[triPtIter]) <
								XMVectorGetX(vLightCameraOrthographicMax))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else if (frustumPlaneIter == 2)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (XMVectorGetY(triangleList[triIter].pt[triPtIter]) >
								XMVectorGetY(vLightCameraOrthographicMin))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (XMVectorGetY(triangleList[triIter].pt[triPtIter]) <
								XMVectorGetY(vLightCameraOrthographicMax))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}

					// Move the points that pass the frustum test to the begining of the array.
					if (iPointPassesCollision[1] && !iPointPassesCollision[0])
					{
						tempOrder = triangleList[triIter].pt[0];
						triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = tempOrder;
						iPointPassesCollision[0] = TRUE;
						iPointPassesCollision[1] = FALSE;
					}
					if (iPointPassesCollision[2] && !iPointPassesCollision[1])
					{
						tempOrder = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = triangleList[triIter].pt[2];
						triangleList[triIter].pt[2] = tempOrder;
						iPointPassesCollision[1] = TRUE;
						iPointPassesCollision[2] = FALSE;
					}
					if (iPointPassesCollision[1] && !iPointPassesCollision[0])
					{
						tempOrder = triangleList[triIter].pt[0];
						triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = tempOrder;
						iPointPassesCollision[0] = TRUE;
						iPointPassesCollision[1] = FALSE;
					}

					if (iInsideVertCount == 0)
					{ // All points failed. We're done,  
						triangleList[triIter].culled = true;
					}
					else if (iInsideVertCount == 1)
					{// One point passed. Clip the triangle against the Frustum plane
						triangleList[triIter].culled = false;

						// 
						XMVECTOR vVert0ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[0];
						XMVECTOR vVert0ToVert2 = triangleList[triIter].pt[2] - triangleList[triIter].pt[0];

						// Find the collision ratio.
						FLOAT fHitPointTimeRatio = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[0], iComponent);
						// Calculate the distance along the vector as ratio of the hit ratio to the component.
						FLOAT fDistanceAlongVector01 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert1, iComponent);
						FLOAT fDistanceAlongVector02 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert2, iComponent);
						// Add the point plus a percentage of the vector.
						vVert0ToVert1 *= fDistanceAlongVector01;
						vVert0ToVert1 += triangleList[triIter].pt[0];
						vVert0ToVert2 *= fDistanceAlongVector02;
						vVert0ToVert2 += triangleList[triIter].pt[0];

						triangleList[triIter].pt[1] = vVert0ToVert2;
						triangleList[triIter].pt[2] = vVert0ToVert1;

					}
					else if (iInsideVertCount == 2)
					{ // 2 in  // tesselate into 2 triangles


						// Copy the triangle\(if it exists) after the current triangle out of
						// the way so we can override it with the new triangle we're inserting.
						triangleList[iTriangleCnt] = triangleList[triIter + 1];

						triangleList[triIter].culled = false;
						triangleList[triIter + 1].culled = false;

						// Get the vector from the outside point into the 2 inside points.
						XMVECTOR vVert2ToVert0 = triangleList[triIter].pt[0] - triangleList[triIter].pt[2];
						XMVECTOR vVert2ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[2];

						// Get the hit point ratio.
						FLOAT fHitPointTime_2_0 = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[2], iComponent);
						FLOAT fDistanceAlongVector_2_0 = fHitPointTime_2_0 / XMVectorGetByIndex(vVert2ToVert0, iComponent);
						// Calcaulte the new vert by adding the percentage of the vector plus point 2.
						vVert2ToVert0 *= fDistanceAlongVector_2_0;
						vVert2ToVert0 += triangleList[triIter].pt[2];

						// Add a new triangle.
						triangleList[triIter + 1].pt[0] = triangleList[triIter].pt[0];
						triangleList[triIter + 1].pt[1] = triangleList[triIter].pt[1];
						triangleList[triIter + 1].pt[2] = vVert2ToVert0;

						//Get the hit point ratio.
						FLOAT fHitPointTime_2_1 = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[2], iComponent);
						FLOAT fDistanceAlongVector_2_1 = fHitPointTime_2_1 / XMVectorGetByIndex(vVert2ToVert1, iComponent);
						vVert2ToVert1 *= fDistanceAlongVector_2_1;
						vVert2ToVert1 += triangleList[triIter].pt[2];
						triangleList[triIter].pt[0] = triangleList[triIter + 1].pt[1];
						triangleList[triIter].pt[1] = triangleList[triIter + 1].pt[2];
						triangleList[triIter].pt[2] = vVert2ToVert1;
						// Cncrement triangle count and skip the triangle we just inserted.
						++iTriangleCnt;
						++triIter;


					}
					else
					{ // all in
						triangleList[triIter].culled = false;

					}
				}// end if !culled loop            
			}
		}
		for (INT index = 0; index < iTriangleCnt; ++index)
		{
			if (!triangleList[index].culled)
			{
				// Set the near and far plan and the min and max z values respectivly.
				for (int vertind = 0; vertind < 3; ++vertind)
				{
					float fTriangleCoordZ = XMVectorGetZ(triangleList[index].pt[vertind]);
					if (fNearPlane > fTriangleCoordZ)
					{
						fNearPlane = fTriangleCoordZ;
					}
					if (fFarPlane < fTriangleCoordZ)
					{
						fFarPlane = fTriangleCoordZ;
					}
				}
			}
		}
	}

}

//--------------------------------------------------------------------------------------
// This function takes the camera's projection matrix and returns the 8
// points that make up a view frustum.
// The frustum is scaled to fit within the Begin and End interval paramaters.
//--------------------------------------------------------------------------------------
void CreateFrustumPointsFromCascadeInterval(float fCascadeIntervalBegin,
	FLOAT fCascadeIntervalEnd,
	CXMMATRIX vProjection,
	XMVECTOR* pvCornerPointsWorld)
{

	BoundingFrustum vViewFrust(vProjection);
	vViewFrust.Near = fCascadeIntervalBegin;
	vViewFrust.Far = fCascadeIntervalEnd;

	static const XMVECTORU32 vGrabY = { 0x00000000,0xFFFFFFFF,0x00000000,0x00000000 };
	static const XMVECTORU32 vGrabX = { 0xFFFFFFFF,0x00000000,0x00000000,0x00000000 };

	XMVECTORF32 vRightTop = { vViewFrust.RightSlope,vViewFrust.TopSlope,1.0f,1.0f };
	XMVECTORF32 vLeftBottom = { vViewFrust.LeftSlope,vViewFrust.BottomSlope,1.0f,1.0f };
	XMVECTORF32 vNear = { vViewFrust.Near,vViewFrust.Near,vViewFrust.Near,1.0f };
	XMVECTORF32 vFar = { vViewFrust.Far,vViewFrust.Far,vViewFrust.Far,1.0f };
	XMVECTOR vRightTopNear = XMVectorMultiply(vRightTop, vNear);
	XMVECTOR vRightTopFar = XMVectorMultiply(vRightTop, vFar);
	XMVECTOR vLeftBottomNear = XMVectorMultiply(vLeftBottom, vNear);
	XMVECTOR vLeftBottomFar = XMVectorMultiply(vLeftBottom, vFar);

	pvCornerPointsWorld[0] = vRightTopNear;
	pvCornerPointsWorld[1] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabX);
	pvCornerPointsWorld[2] = vLeftBottomNear;
	pvCornerPointsWorld[3] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabY);

	pvCornerPointsWorld[4] = vRightTopFar;
	pvCornerPointsWorld[5] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabX);
	pvCornerPointsWorld[6] = vLeftBottomFar;
	pvCornerPointsWorld[7] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabY);

}


void UpdateShadows(FLOAT nearClip, FLOAT farClip, const XMMATRIX& matViewCameraProjection, const XMMATRIX& matViewCameraView, const XMMATRIX& matLightCameraView)
{
	XMMATRIX matInverseViewCamera = XMMatrixInverse(nullptr, matViewCameraView);

	// Convert from min max representation to center extents represnetation.
	// This will make it easier to pull the points out of the transformation.
	BoundingBox bb;
	BoundingBox::CreateFromPoints(bb, m_vSceneAABBMin, m_vSceneAABBMax);

	XMFLOAT3 tmp[8];
	bb.GetCorners(tmp);

	// Transform the scene AABB to Light space.
	XMVECTOR vSceneAABBPointsLightSpace[8];
	for (int index = 0; index < 8; ++index)
	{
		XMVECTOR v = XMLoadFloat3(&tmp[index]);
		vSceneAABBPointsLightSpace[index] = XMVector3Transform(v, matLightCameraView);
	}

	FLOAT fFrustumIntervalBegin, fFrustumIntervalEnd;
	XMVECTOR vLightCameraOrthographicMin;  // light space frustrum aabb 
	XMVECTOR vLightCameraOrthographicMax;
	FLOAT fCameraNearFarRange = farClip - nearClip;

	XMVECTOR vWorldUnitsPerTexel = g_vZero;

	// We loop over the cascades to calculate the orthographic projection for each cascade.
	for (INT iCascadeIndex = 0; iCascadeIndex < m_CopyOfCascadeConfig.m_nCascadeLevels; ++iCascadeIndex)
	{
		// Calculate the interval of the View Frustum that this cascade covers. We measure the interval 
		// the cascade covers as a Min and Max distance along the Z Axis.
		//if (m_eSelectedCascadesFit == FIT_TO_CASCADES)
		//{
		//	// Because we want to fit the orthogrpahic projection tightly around the Cascade, we set the Mimiumum cascade 
		//	// value to the previous Frustum end Interval
		//	if (iCascadeIndex == 0) fFrustumIntervalBegin = 0.0f;
		//	else fFrustumIntervalBegin = (FLOAT)m_iCascadePartitionsZeroToOne[iCascadeIndex - 1];
		//}
		//else
		{
			// In the FIT_TO_SCENE technique the Cascades overlap eachother.  In other words, interval 1 is coverd by
			// cascades 1 to 8, interval 2 is covered by cascades 2 to 8 and so forth.
			fFrustumIntervalBegin = 0.0f;
		}

		// Scale the intervals between 0 and 1. They are now percentages that we can scale with.
		fFrustumIntervalEnd = (FLOAT)m_iCascadePartitionsZeroToOne[iCascadeIndex];
		fFrustumIntervalBegin /= (FLOAT)m_iCascadePartitionsMax;
		fFrustumIntervalEnd /= (FLOAT)m_iCascadePartitionsMax;
		fFrustumIntervalBegin = fFrustumIntervalBegin * fCameraNearFarRange;
		fFrustumIntervalEnd = fFrustumIntervalEnd * fCameraNearFarRange;
		XMVECTOR vFrustumPoints[8];

		// This function takes the began and end intervals along with the projection matrix and returns the 8
		// points that repreresent the cascade Interval
		CreateFrustumPointsFromCascadeInterval(fFrustumIntervalBegin, fFrustumIntervalEnd,
			matViewCameraProjection, vFrustumPoints);

		vLightCameraOrthographicMin = g_vFLTMAX;
		vLightCameraOrthographicMax = g_vFLTMIN;

		XMVECTOR vTempTranslatedCornerPoint;
		// This next section of code calculates the min and max values for the orthographic projection.
		for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
		{
			// Transform the frustum from camera view space to world space.
			vFrustumPoints[icpIndex] = XMVector4Transform(vFrustumPoints[icpIndex], matInverseViewCamera);
			// Transform the point from world space to Light Camera Space.
			vTempTranslatedCornerPoint = XMVector4Transform(vFrustumPoints[icpIndex], matLightCameraView);
			// Find the closest point.
			vLightCameraOrthographicMin = XMVectorMin(vTempTranslatedCornerPoint, vLightCameraOrthographicMin);
			vLightCameraOrthographicMax = XMVectorMax(vTempTranslatedCornerPoint, vLightCameraOrthographicMax);
		}

		// This code removes the shimmering effect along the edges of shadows due to
		// the light changing to fit the camera.
		//if (m_eSelectedCascadesFit == FIT_TO_SCENE)
		{
			// Fit the ortho projection to the cascades far plane and a near plane of zero. 
			// Pad the projection to be the size of the diagonal of the Frustum partition. 
			// 
			// To do this, we pad the ortho transform so that it is always big enough to cover 
			// the entire camera view frustum.
			XMVECTOR vDiagonal = vFrustumPoints[0] - vFrustumPoints[6];
			vDiagonal = XMVector3Length(vDiagonal);

			// The bound is the length of the diagonal of the frustum interval.
			FLOAT fCascadeBound = XMVectorGetX(vDiagonal);

			// The offset calculated will pad the ortho projection so that it is always the same size 
			// and big enough to cover the entire cascade interval.
			XMVECTOR vBoarderOffset = (vDiagonal -
				(vLightCameraOrthographicMax - vLightCameraOrthographicMin))
				* g_vHalfVector;
			// Set the Z and W components to zero.
			vBoarderOffset *= g_vMultiplySetzwToZero;

			// Add the offsets to the projection.
			vLightCameraOrthographicMax += vBoarderOffset;
			vLightCameraOrthographicMin -= vBoarderOffset;

			// The world units per texel are used to snap the shadow the orthographic projection
			// to texel sized increments.  This keeps the edges of the shadows from shimmering.
			FLOAT fWorldUnitsPerTexel = fCascadeBound / (float)m_CopyOfCascadeConfig.m_iBufferSize;
			vWorldUnitsPerTexel = XMVectorSet(fWorldUnitsPerTexel, fWorldUnitsPerTexel, 0.0f, 0.0f);


		}
		//else if (m_eSelectedCascadesFit == FIT_TO_CASCADES)
		//{

		//	// We calculate a looser bound based on the size of the PCF blur.  This ensures us that we're 
		//	// sampling within the correct map.
		//	float fScaleDuetoBlureAMT = ((float)(m_iPCFBlurSize * 2 + 1)
		//		/ (float)m_CopyOfCascadeConfig.m_iBufferSize);
		//	XMVECTORF32 vScaleDuetoBlureAMT = { fScaleDuetoBlureAMT, fScaleDuetoBlureAMT, 0.0f, 0.0f };


		//	float fNormalizeByBufferSize = (1.0f / (float)m_CopyOfCascadeConfig.m_iBufferSize);
		//	XMVECTOR vNormalizeByBufferSize = XMVectorSet(fNormalizeByBufferSize, fNormalizeByBufferSize, 0.0f, 0.0f);

		//	// We calculate the offsets as a percentage of the bound.
		//	XMVECTOR vBoarderOffset = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
		//	vBoarderOffset *= g_vHalfVector;
		//	vBoarderOffset *= vScaleDuetoBlureAMT;
		//	vLightCameraOrthographicMax += vBoarderOffset;
		//	vLightCameraOrthographicMin -= vBoarderOffset;

		//	// The world units per texel are used to snap  the orthographic projection
		//	// to texel sized increments.  
		//	// Because we're fitting tighly to the cascades, the shimmering shadow edges will still be present when the 
		//	// camera rotates.  However, when zooming in or strafing the shadow edge will not shimmer.
		//	vWorldUnitsPerTexel = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
		//	vWorldUnitsPerTexel *= vNormalizeByBufferSize;

		//}
		//float fLightCameraOrthographicMinZ = XMVectorGetZ(vLightCameraOrthographicMin);


		//if (m_bMoveLightTexelSize)
		{

			// We snape the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
			// This is a matter of integer dividing by the world space size of a texel
			vLightCameraOrthographicMin /= vWorldUnitsPerTexel;
			vLightCameraOrthographicMin = XMVectorFloor(vLightCameraOrthographicMin);
			vLightCameraOrthographicMin *= vWorldUnitsPerTexel;

			vLightCameraOrthographicMax /= vWorldUnitsPerTexel;
			vLightCameraOrthographicMax = XMVectorFloor(vLightCameraOrthographicMax);
			vLightCameraOrthographicMax *= vWorldUnitsPerTexel;

		}

		//These are the unconfigured near and far plane values.  They are purposly awful to show 
		// how important calculating accurate near and far planes is.
		FLOAT fNearPlane = 0.0f;
		FLOAT fFarPlane = 10000.0f;

		//if (m_eSelectedNearFarFit == FIT_NEARFAR_AABB)
		//{

		//	XMVECTOR vLightSpaceSceneAABBminValue = g_vFLTMAX;  // world space scene aabb 
		//	XMVECTOR vLightSpaceSceneAABBmaxValue = g_vFLTMIN;
		//	// We calculate the min and max vectors of the scene in light space. The min and max "Z" values of the  
		//	// light space AABB can be used for the near and far plane. This is easier than intersecting the scene with the AABB
		//	// and in some cases provides similar results.
		//	for (int index = 0; index < 8; ++index)
		//	{
		//		vLightSpaceSceneAABBminValue = XMVectorMin(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBminValue);
		//		vLightSpaceSceneAABBmaxValue = XMVectorMax(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBmaxValue);
		//	}

		//	// The min and max z values are the near and far planes.
		//	fNearPlane = XMVectorGetZ(vLightSpaceSceneAABBminValue);
		//	fFarPlane = XMVectorGetZ(vLightSpaceSceneAABBmaxValue);
		//}
		//else if (m_eSelectedNearFarFit == FIT_NEARFAR_SCENE_AABB
		//	|| m_eSelectedNearFarFit == FIT_NEARFAR_PANCAKING)
		{
			// By intersecting the light frustum with the scene AABB we can get a tighter bound on the near and far plane.
			ComputeNearAndFar(fNearPlane, fFarPlane, vLightCameraOrthographicMin,
				vLightCameraOrthographicMax, vSceneAABBPointsLightSpace);
			//if (m_eSelectedNearFarFit == FIT_NEARFAR_PANCAKING)
			//{
			//	if (fLightCameraOrthographicMinZ > fNearPlane)
			//	{
			//		fNearPlane = fLightCameraOrthographicMinZ;
			//	}
			//}
		}
		// Create the orthographic projection for this cascade.
		m_matShadowProj[iCascadeIndex] = XMMatrixOrthographicOffCenterLH(XMVectorGetX(vLightCameraOrthographicMin), XMVectorGetX(vLightCameraOrthographicMax),
			XMVectorGetY(vLightCameraOrthographicMin), XMVectorGetY(vLightCameraOrthographicMax),
			fNearPlane, fFarPlane);
		m_fCascadePartitionsFrustum[iCascadeIndex] = fFrustumIntervalEnd;
	}
}
#endif