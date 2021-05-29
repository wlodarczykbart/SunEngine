#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/matrix.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <DirectXMath.h>

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
#include "RenderTarget.h"

using namespace SunEngine;
using namespace glm;

void updateCascades(float nearClip, float farClip, float cascadeSplitLambda, const mat4& viewProj, const vec3& lightPos, mat4& shadowMatrix);

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
		Vector<vec4> verts = {
			vec4(-1.0f, -1.0f, +1.0f, +1.0f),
			vec4(+1.0f, -1.0f, +1.0f, +1.0f),
			vec4(+1.0f, +1.0f, +1.0f, +1.0f),
			vec4(-1.0f, +1.0f, +1.0f, +1.0f),
			vec4(-1.0f, -1.0f, -1.0f, +1.0f),
			vec4(+1.0f, -1.0f, -1.0f, +1.0f),
			vec4(+1.0f, +1.0f, -1.0f, +1.0f),
			vec4(-1.0f, +1.0f, -1.0f, +1.0f),
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
		info.vertexStride = sizeof(vec4);
		info.numVerts = verts.size();
		info.pVerts = verts.data();
		info.numIndices = indices.size();
		info.pIndices = indices.data();

		if (!cube.Create(info))
			return -1;
	}

	BaseMesh plane;
	{
		Vector<vec4> verts = {
			vec4(-10.0f, 0.0f, +10.0f, 1.0f),
			vec4(+10.0f, 0.0f, +10.0f, 1.0f),
			vec4(+10.0f, 0.0f, -10.0f, 1.0f),
			vec4(-10.0f, 0.0f, -10.0f, 1.0f),
		};

		Vector<uint> indices = {
			0, 1, 2,
			0, 2, 3,
		};

		BaseMesh::CreateInfo info = {};
		info.vertexStride = sizeof(vec4);
		info.numVerts = verts.size();
		info.pVerts = verts.data();
		info.numIndices = indices.size();
		info.pIndices = indices.data();

		if (!plane.Create(info))
			return false;
	}

	struct MatrixBuffer
	{
		mat4 ViewProjMatrix;
		mat4 WorldMatirx;
		mat4 ShadowMatrix;
	};
	
	IShaderBuffer shaderMtxBuffer = {};
	strncpy_s(shaderMtxBuffer.name, "MatrixBuffer", strlen("MatrixBuffer"));
	shaderMtxBuffer.bindType = SBT_CAMERA;
	shaderMtxBuffer.stages = SS_VERTEX | SS_PIXEL;
	shaderMtxBuffer.numVariables = sizeof(MatrixBuffer) / sizeof(mat4);
	shaderMtxBuffer.size = sizeof(MatrixBuffer);
	shaderMtxBuffer.binding[SE_GFX_D3D11] = 0;
	shaderMtxBuffer.binding[SE_GFX_VULKAN] = 0;

	const char* SHADER_BUFFER_VAR_NAMES[3] = { "ViewProjMatrix", "WorldMatrix", "ShadowMatrix" };
	for (uint i = 0; i < shaderMtxBuffer.numVariables; i++)
	{
		strncpy_s(shaderMtxBuffer.variables[i].name, SHADER_BUFFER_VAR_NAMES[i], strlen(SHADER_BUFFER_VAR_NAMES[i]));
		shaderMtxBuffer.variables[i].size = sizeof(mat4);
		shaderMtxBuffer.variables[i].offset = sizeof(mat4) * i;
		shaderMtxBuffer.variables[i].type = SDT_MAT4;
	}

	String shaderPath = "";

	BaseShader coreShader, depthShader;
	{
		BaseShader::CreateInfo info = {};
		info.buffers[shaderMtxBuffer.name] = shaderMtxBuffer;		
		
		IVertexElement vtxElem;
		vtxElem.format = VIF_FLOAT4;
		vtxElem.offset = 0;
		vtxElem.size = sizeof(vec4);
		strncpy_s(vtxElem.semantic, "POSITION", strlen("POSITION"));
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
		resource.binding[SE_GFX_D3D11] = 0;
		resource.bindingCount = 1;
		resource.bindType = SBT_OBJECT;
		resource.stages = SS_PIXEL;

		strncpy_s(resource.name, "DepthTexture", strlen("DepthTexture"));
		resource.dimension = SRD_TEXTURE_2D;
		resource.type = SRT_TEXTURE;
		info.resources[resource.name] = resource;

		strncpy_s(resource.name, "Sampler", strlen("Sampler"));
		resource.type = SRT_SAMPLER;
		info.resources[resource.name] = resource;

		if (!coreShader.Create(info))
			return -1;
	}

	ShaderBindings shaderBindings, shadowBindings;
	{
		ShaderBindings::CreateInfo info = {};
		info.pShader = &coreShader;
		info.type = shaderMtxBuffer.bindType;
		if (!shaderBindings.Create(info))
			return -1;

		info.pShader = &coreShader;
		info.type = SBT_OBJECT;
		if (!shadowBindings.Create(info))
			return -1;
	}

	UniformBuffer shaderUniformBuffer;
	{
		UniformBuffer::CreateInfo info = {};
		info.isShared = true;
		info.size = shaderMtxBuffer.size;
		if (!shaderUniformBuffer.Create(info))
			return - 1;
	}

	if (!shaderBindings.SetUniformBuffer(shaderMtxBuffer.name, &shaderUniformBuffer))
		return -1;

	GraphicsPipeline corePipeline, depthPipeline;
	{
		GraphicsPipeline::CreateInfo info = {};

		info.pShader = &coreShader;
		if (!corePipeline.Create(info))
			return -1;

		info.pShader = &depthShader;
		if (!depthPipeline.Create(info))
			return -1;
	}

	RenderTarget depthTarget;
	{
		RenderTarget::CreateInfo info = {};
		info.width = 512;
		info.height = 512;
		info.hasDepthBuffer = true;

		if (!depthTarget.Create(info))
			return -1;
	}

	Sampler sampler;
	{
		Sampler::CreateInfo info = {};
		info.settings.anisotropicMode = SE_AM_OFF;
		info.settings.filterMode = SE_FM_LINEAR;
		info.settings.wrapMode = SE_WM_CLAMP_TO_EDGE;
		if (!sampler.Create(info))
			return -1;
	}

	if(	!shadowBindings.SetTexture("DepthTexture", depthTarget.GetDepthTexture()))
		return -1;
	if (!shadowBindings.SetSampler("Sampler", &sampler))
		return -1;

	auto* cmdBuffer = surface.GetCommandBuffer();

	mat4 iden = mat4(1.0f);

	float nearClip = 0.5f;
	float farClip = 500.0f;
	mat4 proj = perspective(glm::radians(45.0f), 1.0f, nearClip, farClip);

	MatrixBuffer mtxBuffers[2];
	mtxBuffers[0].ShadowMatrix = mtxBuffers[1].ShadowMatrix = iden;

	mtxBuffers[0].WorldMatirx = translate(iden, vec3(0.0f, 1.0f, 0.0f));
	mtxBuffers[1].WorldMatirx = mat4(1.0f);

	IShaderBindingsBindState bindState = {};
	bindState.DynamicIndices[0].first = shaderMtxBuffer.name;

	ivec2 mousePos(0);
	vec2 viewAngles(0.0f, -10.0f);
	vec3 viewPosition(0.0f, 3.0, 3.0f);
	float viewSpeed = 0.1f;

	mat4 shadowBis = translate(iden, vec3(0.5f, 0.5f, 0.0f)) * scale(iden, vec3(0.5f, -0.5f, 1.0f));

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
			ivec2 currPos;
			window.GetMousePosition(currPos.x, currPos.y);
			vec2 delta = vec2(currPos.x - mousePos.x, currPos.y - mousePos.y) * viewSpeed;
			viewAngles += delta;
			mousePos = currPos;
		}

		mat4 viewRotation;
		viewRotation = rotate(iden, radians(viewAngles.x), vec3(0.0f, 1.0f, 0.0f)) * rotate(iden, radians(viewAngles.y), vec3(1.0f, 0.0f, 0.0f));

		if (window.KeyDown(KEY_A)) 
			viewPosition -= vec3(viewRotation[0]) * viewSpeed;
		if (window.KeyDown(KEY_D)) 
			viewPosition += vec3(viewRotation[0]) * viewSpeed;
		if (window.KeyDown(KEY_W)) 
			viewPosition -= vec3(viewRotation[2]) * viewSpeed;
		if (window.KeyDown(KEY_S)) 
			viewPosition += vec3(viewRotation[2]) * viewSpeed;
		mat4 viewProj = proj * inverse(translate(iden, viewPosition) * viewRotation);

		mat4 shadowMatrix;
		updateCascades(nearClip, farClip, 1.0f, viewProj, vec3(14.0f, 20.0f, -10.0f), shadowMatrix);

		surface.StartFrame();

		mtxBuffers[0].ViewProjMatrix = shadowMatrix;
		shaderUniformBuffer.UpdateShared(mtxBuffers, sizeof(mtxBuffers) / sizeof(*mtxBuffers));

		depthTarget.Bind(cmdBuffer);
		depthShader.Bind(cmdBuffer);
		depthPipeline.Bind(cmdBuffer);
		cube.Bind(cmdBuffer);
		bindState.DynamicIndices[0].second = 0;
		shaderBindings.Bind(cmdBuffer, &bindState);
		cmdBuffer->DrawIndexed(cube.GetNumIndices(), 1, 0, 0, 0);
		depthTarget.Unbind(cmdBuffer);

		mtxBuffers[0].ViewProjMatrix = viewProj;
		mtxBuffers[1].ViewProjMatrix = viewProj;

		shadowMatrix = shadowBis * shadowMatrix;
		mtxBuffers[0].ShadowMatrix = shadowMatrix;
		mtxBuffers[1].ShadowMatrix = shadowMatrix;
		shaderUniformBuffer.UpdateShared(mtxBuffers, sizeof(mtxBuffers) / sizeof(*mtxBuffers));

		surface.Bind(cmdBuffer);

		coreShader.Bind(cmdBuffer);
		corePipeline.Bind(cmdBuffer);
		shadowBindings.Bind(cmdBuffer);

		cube.Bind(cmdBuffer);
		bindState.DynamicIndices[0].second = 0;
		shaderBindings.Bind(cmdBuffer, &bindState);
		cmdBuffer->DrawIndexed(cube.GetNumIndices(), 1, 0, 0, 0);

		plane.Bind(cmdBuffer);
		bindState.DynamicIndices[0].second = 1;
		shaderBindings.Bind(cmdBuffer, &bindState);
		cmdBuffer->DrawIndexed(plane.GetNumIndices(), 1, 0, 0, 0);

		shadowBindings.Unbind(cmdBuffer);
		corePipeline.Unbind(cmdBuffer);
		coreShader.Unbind(cmdBuffer);

		surface.Unbind(cmdBuffer);
		surface.EndFrame();
	}


	return 0;
}

void updateCascades(float nearClip, float farClip, float cascadeSplitLambda, const mat4& viewProj, const vec3& lightPos, mat4& shadowMatrix)
{
	const uint SHADOW_MAP_CASCADE_COUNT = 1;
	float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

	float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		cascadeSplits[i] = (d - nearClip) / clipRange;
	}

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float splitDist = cascadeSplits[i];

		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, -1.0f),
			glm::vec3(1.0f,  1.0f, -1.0f),
			glm::vec3(1.0f, -1.0f, -1.0f),
			glm::vec3(-1.0f, -1.0f, -1.0f),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		glm::mat4 invCam = glm::inverse(viewProj);
		for (uint32_t i = 0; i < 8; i++) {
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
			frustumCorners[i] = invCorner / invCorner.w;
		}

		for (uint32_t i = 0; i < 4; i++) {
			glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
			frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
			frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t i = 0; i < 8; i++) {
			frustumCenter += frustumCorners[i];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t i = 0; i < 8; i++) {
			float distance = glm::length(frustumCorners[i] - frustumCenter);
			radius = max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		glm::vec3 lightDir = normalize(-lightPos);
		glm::vec3 lightLookPos = frustumCenter - lightDir * -minExtents.z;
		glm::mat4 lightViewMatrix = glm::lookAt(lightLookPos, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

		// Store split distance and matrix in cascade
		//cascades[i].splitDepth = (camera.getNearClip() + splitDist * clipRange) * -1.0f;
		//cascades[i].viewProjMatrix = lightOrthoMatrix * lightViewMatrix;
		shadowMatrix = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = cascadeSplits[i];
	}

	if(false)
	{
		using namespace DirectX;

		struct
		{
			XMFLOAT3 Center;
			float Radius;
		} mSceneBounds;

		mSceneBounds.Center = { 0.0f, 0.0f, 0.0f };
		mSceneBounds.Radius = 5.0f;

		XMFLOAT3 dirLight = { -0.577260733, -0.577350020, 0.577439308 };

		// Only the first "main" light casts a shadow.
		XMVECTOR lightDir = XMLoadFloat3(&dirLight);
		XMVECTOR lightPos = -2.0f * mSceneBounds.Radius * lightDir;
		XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMMATRIX V = XMMatrixLookAtLH(lightPos, targetPos, up);

		// Transform bounding sphere to light space.
		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, V));

		// Ortho frustum in light space encloses scene.
		float l = sphereCenterLS.x - mSceneBounds.Radius;
		float b = sphereCenterLS.y - mSceneBounds.Radius;
		float n = sphereCenterLS.z - mSceneBounds.Radius;
		float r = sphereCenterLS.x + mSceneBounds.Radius;
		float t = sphereCenterLS.y + mSceneBounds.Radius;
		float f = sphereCenterLS.z + mSceneBounds.Radius;
		XMMATRIX P = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

		//mCam.SetLens(mCam.GetFovY(), mCam.GetAspect(), 1.0f, 100.0f);
		//XMMATRIX invView = XMMatrixInverse(NULL, mCam.View());
		//XMMATRIX invProj = XMMatrixInverse(NULL, mCam.Proj());

		//XMVECTOR frustum[8]
		//{
		//	XMVectorSet(-1, -1, -1, 1),
		//	XMVectorSet(+1, -1, -1, 1),
		//	XMVectorSet(-1, +1, -1, 1),
		//	XMVectorSet(+1, +1, -1, 1),
		//	XMVectorSet(-1, -1, +1, 1),
		//	XMVectorSet(+1, -1, +1, 1),
		//	XMVectorSet(-1, +1, +1, 1),
		//	XMVectorSet(+1, +1, +1, 1),
		//};

		//XMVECTOR vMin = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
		//XMVECTOR vMax = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

		//for (int i = 0; i < 8; i++)
		//{
		//	XMVECTOR v = frustum[i];
		//	v = XMVector4Transform(v, invProj);
		//	float w = v.m128_f32[3];
		//	v = XMVectorDivide(v, XMVectorSet(w, w, w, w));
		//	v = XMVector4Transform(v, invView);

		//	vMin = XMVectorMin(v, vMin);
		//	vMax = XMVectorMax(v, vMax);
		//	
		//	frustum[i] = v;

		//}

		//XMFLOAT4 minExtent, maxExtent;
		//XMStoreFloat4(&minExtent, vMin);
		//XMStoreFloat4(&maxExtent, vMax);

		//float radius = maxExtent.z - minExtent.z;

		//P = XMMatrixOrthographicOffCenterLH(minExtent.x, maxExtent.x, minExtent.y, maxExtent.y, minExtent.z, maxExtent.z);

		// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
		XMMATRIX T(
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);

		XMMATRIX S = V * P * T;

		XMMATRIX ViewProj = V * P;
		shadowMatrix = *reinterpret_cast<mat4*>(&ViewProj);

		//XMStoreFloat4x4(&mLightView, V);
		//XMStoreFloat4x4(&mLightProj, P);
		//XMStoreFloat4x4(&mShadowTransform, S);
	}
}