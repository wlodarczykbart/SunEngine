#define TEST_SCREEN_SPACE_REFLECTION

#ifdef TEST_SCREEN_SPACE_REFLECTION

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
		buffer.stages = SS_VERTEX | SS_PIXEL;

		variables =
		{
			ShaderVarDef("ViewProjMatrix", SDT_MAT4),
			ShaderVarDef("ViewMatrix", SDT_MAT4),
			ShaderVarDef("ProjMatrix", SDT_MAT4),
			ShaderVarDef("InvProjMatrix", SDT_MAT4),
			ShaderVarDef("ProjPixelMatrix", SDT_MAT4),
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
	}

	Map<ShaderBindingType, ShaderBindingData> bindingMap;
	FillShaderBufferInfo(SBT_CAMERA, bindingMap[SBT_CAMERA].bufferInfo);
	FillShaderBufferInfo(SBT_OBJECT, bindingMap[SBT_OBJECT].bufferInfo);

	//char path[2048];
	//GetCurrentDirectory(sizeof(path), path);
	//String shaderPath = path;
	//shaderPath += "/";

	String shaderPath = "";
	
	BaseShader coreShader, ssrShader;
	{
		BaseShader::CreateInfo info = {};
		info.buffers[bindingMap[SBT_CAMERA].bufferInfo.name] = bindingMap[SBT_CAMERA].bufferInfo;
		info.buffers[bindingMap[SBT_OBJECT].bufferInfo.name] = bindingMap[SBT_OBJECT].bufferInfo;

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

		fs.OpenForRead((shaderPath + "ssr_core.ps.cso").c_str());
		fs.ReadBuffer(info.pixelBinaries[SE_GFX_D3D11]);
		fs.Close();

		if (!coreShader.Create(info))
			return -1;

		info.vertexElements.clear();

		fs.OpenForRead((shaderPath + "ssr.vs.cso").c_str());
		fs.ReadBuffer(info.vertexBinaries[SE_GFX_D3D11]);
		fs.Close();

		fs.OpenForRead((shaderPath + "ssr.ps.cso").c_str());
		fs.ReadBuffer(info.pixelBinaries[SE_GFX_D3D11]);
		fs.Close();

		IShaderResource resource = {};
		resource.bindingCount = 1;
		resource.bindType = SBT_MATERIAL;
		resource.stages = SS_PIXEL;

		sprintf_s(resource.name, "ColorTexture");
		resource.dimension = SRD_TEXTURE_2D;
		resource.type = SRT_TEXTURE;
		resource.binding[SE_GFX_D3D11] = 0;
		//resource.binding[SE_GFX_VULKAN] = 0;
		info.resources[resource.name] = resource;

		sprintf_s(resource.name, "PositionTexture");
		resource.dimension = SRD_TEXTURE_2D;
		resource.type = SRT_TEXTURE;
		resource.binding[SE_GFX_D3D11] = 1;
		//resource.binding[SE_GFX_VULKAN] = 0;
		info.resources[resource.name] = resource;

		sprintf_s(resource.name, "NormalTexture");
		resource.dimension = SRD_TEXTURE_2D;
		resource.type = SRT_TEXTURE;
		resource.binding[SE_GFX_D3D11] = 2;
		//resource.binding[SE_GFX_VULKAN] = 0;
		info.resources[resource.name] = resource;

		sprintf_s(resource.name, "DepthTexture");
		resource.dimension = SRD_TEXTURE_2D;
		resource.type = SRT_TEXTURE;
		resource.binding[SE_GFX_D3D11] = 3;
		//resource.binding[SE_GFX_VULKAN] = 0;
		info.resources[resource.name] = resource;

		sprintf_s(resource.name, "Sampler");
		resource.binding[SE_GFX_D3D11] = 0;
		//resource.binding[SE_GFX_VULKAN] = 0;
		resource.type = SRT_SAMPLER;
		info.resources[resource.name] = resource;

		if (!ssrShader.Create(info))
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

	GraphicsPipeline corePipeline, ssrPipeline;
	{
		GraphicsPipeline::CreateInfo info = {};
#ifdef GLM_FORCE_LEFT_HANDED
		info.settings.rasterizer.frontFace = SE_FF_CLOCKWISE;
#endif

		info.pShader = &coreShader;
		if (!corePipeline.Create(info))
			return -1;

		info.pShader = &ssrShader;
		if (!ssrPipeline.Create(info))
			return -1;
	}

	RenderTarget colorTarget;
	{
		RenderTarget::CreateInfo info = {};
		info.width = window.Width();
		info.height = window.Height();
		info.numTargets = 3;
		info.hasDepthBuffer = true;
		info.floatingPointColorBuffer = true;

		if (!colorTarget.Create(info))
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

	ShaderBindings ssrBindings;
	{
		ShaderBindings::CreateInfo info = {};
		info.pShader = &ssrShader;
		info.type = SBT_MATERIAL;
		if (!ssrBindings.Create(info))
			return -1;

		if (!ssrBindings.SetTexture("ColorTexture", colorTarget.GetColorTexture(0)))
			return -1;
		if (!ssrBindings.SetTexture("PositionTexture", colorTarget.GetColorTexture(1)))
			return -1;
		if (!ssrBindings.SetTexture("NormalTexture", colorTarget.GetColorTexture(2)))
			return -1;
		if (!ssrBindings.SetTexture("DepthTexture", colorTarget.GetDepthTexture()))
			return -1;
		if (!ssrBindings.SetSampler("Sampler", &sampler))
			return -1;
	}

	auto* cmdBuffer = surface.GetCommandBuffer();

	float nearClip = 0.5f;
	float farClip = 300.0f;
	glm::mat4 projMatrix = glm::perspective(glm::radians(45.0f), 1.0f, nearClip, farClip);

	float coordinateSystem = -1.0f;
#ifdef GLM_FORCE_LEFT_HANDED
	coordinateSystem = -coordinateSystem;
#endif

	glm::mat4 invProjMatrix = glm::inverse(projMatrix);
	glm::mat4 projPixelMatrix =
		glm::translate(glm::vec3(colorTarget.Width() / 2.0f, float(colorTarget.Height() / 2.0f), 0)) *
		glm::scale(glm::vec3(colorTarget.Width() / 2.0f, -float(colorTarget.Height() / 2.0f), 1.0f));

	projPixelMatrix = projPixelMatrix * projMatrix;

	float depth = 0.98f;
	glm::vec4 ndc = glm::vec4(0.0f, 0.0f, depth, 1.0f);
	ndc = invProjMatrix * ndc;
	ndc /= ndc.w;

	float eye0 = ndc.z;
	float eye1 = projMatrix[2][3] * projMatrix[3][2] / (depth + projMatrix[2][2]);

	glm::ivec2 mousePos(0, 0);
	glm::vec4 viewAngles = glm::vec4(618.0f, -19.0f, 0.0f, 0.0f);
	glm::vec4 viewPosition = glm::vec4(-19.f, 3.8f, -12.8f, 1.0f);
	glm::vec4 viewSpeed = glm::vec4(0.1f, 0.1f, 0.1f, 0.1f);

	struct RenderNode
	{
		BaseMesh* pMesh;
		glm::mat4 worldMatrix;
	};

	FLOAT worldRange = 40.0f;

	Vector<RenderNode> renderNodes;
	uint cubeCount = 30;
	for (uint i = 0; i < cubeCount; i++)
	{
		RenderNode planeNode;
		planeNode.pMesh = &cube;

		glm::vec4 t = glm::vec4(rand() / (float)RAND_MAX, 0.0f, rand() / (float)RAND_MAX, 0.0f);
		glm::vec4 pos = glm::mix(glm::vec4(-worldRange, 0.0f, -worldRange, 0.0f), glm::vec4(worldRange, 0.0f, worldRange, 0.0f), t);
		planeNode.worldMatrix = glm::translate(glm::vec3(pos)) * glm::scale(glm::vec3(1.0f, 1.0f + (20.0f * (rand() / float(RAND_MAX))), 1.0f));
		renderNodes.push_back(planeNode);
	}

	RenderNode planeNode;
	planeNode.pMesh = &plane;
	planeNode.worldMatrix = glm::scale(glm::vec3(worldRange, 1.0f, worldRange));
	renderNodes.push_back(planeNode);

	Vector<glm::mat4> worldMatrices;
	worldMatrices.resize(renderNodes.size());

	IShaderBindingsBindState objectBindState = {};
	objectBindState.DynamicIndices[0].first = bindingMap[SBT_OBJECT].bufferInfo.name;

	IShaderBindingsBindState cameraBindState = {};
	cameraBindState.DynamicIndices[0].first = bindingMap[SBT_CAMERA].bufferInfo.name;

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

		float lightScale = 1.0f;
		glm::mat4 lightMatrix = glm::lookAt(glm::vec3(lightDir) * lightScale, glm::vec3(0), glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 viewMatrix = glm::translate(glm::vec3(viewPosition)) * viewRotation;
		viewMatrix = glm::inverse(viewMatrix);

		glm::mat4 viewProj = projMatrix * viewMatrix;

		struct CameraBufferData
		{
			glm::mat4 ViewProj;
			glm::mat4 View;
			glm::mat4 Proj;
			glm::mat4 InvProj;
			glm::mat4 ProjPixelMatrix;
		} cameraMatrices[1];

		CameraBufferData mainCamData;
		mainCamData.ViewProj = viewProj;
		mainCamData.View = viewMatrix;
		mainCamData.Proj = projMatrix;
		mainCamData.InvProj = invProjMatrix;
		mainCamData.ProjPixelMatrix = projPixelMatrix;
		cameraMatrices[0] =  mainCamData;


		bindingMap[SBT_CAMERA].ubo.UpdateShared(cameraMatrices, 1);

		for (uint i = 0; i < renderNodes.size(); i++)
			worldMatrices[i] = renderNodes[i].worldMatrix;
		bindingMap[SBT_OBJECT].ubo.UpdateShared(worldMatrices.data(), worldMatrices.size());

		surface.StartFrame();

		colorTarget.Bind(cmdBuffer);

		coreShader.Bind(cmdBuffer);
		corePipeline.Bind(cmdBuffer);

		cameraBindState.DynamicIndices[0].second = 0;
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

		corePipeline.Unbind(cmdBuffer);
		coreShader.Unbind(cmdBuffer);

		colorTarget.Unbind(cmdBuffer);

		surface.Bind(cmdBuffer);

		ssrShader.Bind(cmdBuffer);
		ssrPipeline.Bind(cmdBuffer);
		ssrBindings.Bind(cmdBuffer);

		cameraBindState.DynamicIndices[0].second = 0;
		bindingMap[SBT_CAMERA].bindings.Bind(cmdBuffer, &cameraBindState);

		cmdBuffer->Draw(6, 1, 0, 0);

		ssrBindings.Unbind(cmdBuffer);
		ssrPipeline.Unbind(cmdBuffer);
		ssrShader.Unbind(cmdBuffer);

		surface.Unbind(cmdBuffer);

		surface.EndFrame();
	}
	return 0;
}

#endif