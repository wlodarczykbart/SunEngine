#pragma pack_matrix(row_major)
#define APPLY_LIGHTING 1
#define APPLY_FOG 1

[[vk::binding(0, 5)]]
cbuffer ObjectBuffer
 : register(b1){
	float4x4 WorldMatrix;
	float4x4 NormalMatrix;
};

#define DELTA_TIME (TimeData.x)
#define ELAPSED_TIME (TimeData.y)
#define HORIZ_WIND WindVec.xz

[[vk::binding(5, 0)]]
cbuffer EnvBuffer
 : register(b9){
	float4 TimeData;
	float4 WindVec;
};

[[vk::binding(0, 1)]]
Texture2D SceneTexture : register(t0);
[[vk::binding(0, 2)]]
SamplerState SceneSampler : register(s0);
[[vk::binding(1, 1)]]
Texture2D DepthTexture : register(t1);
[[vk::binding(1, 2)]]
SamplerState DepthSampler : register(s1);

[[vk::binding(7, 0)]]
cbuffer SampleSceneBuffer
 : register(b11){
	float4 SampleSceneTexCoordTransform;
	float4 SampleSceneTexCoordRanges;
};

float4 SampleScene(float2 texCoord)
{
	float2 texClamped = texCoord * SampleSceneTexCoordTransform.xy + SampleSceneTexCoordTransform.zw;
	texClamped.x = clamp(texClamped.x, SampleSceneTexCoordRanges.x, SampleSceneTexCoordRanges.z);
	texClamped.y = clamp(texClamped.y, SampleSceneTexCoordRanges.y, SampleSceneTexCoordRanges.w);
	return SceneTexture.Sample(SceneSampler, texClamped);
}

float SampleDepth(float2 texCoord)
{
	float2 texClamped = texCoord * SampleSceneTexCoordTransform.xy + SampleSceneTexCoordTransform.zw;
	texClamped.x = clamp(texClamped.x, SampleSceneTexCoordRanges.x, SampleSceneTexCoordRanges.z);
	texClamped.y = clamp(texClamped.y, SampleSceneTexCoordRanges.y, SampleSceneTexCoordRanges.w);
	return DepthTexture.Sample(DepthSampler, texClamped).r;
}

[[vk::binding(0, 0)]]
cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};

[[vk::binding(1, 0)]]
cbuffer SunlightBuffer
 : register(b3){
	float4 SunDirection;
	float4 SunColor;
};

float3 ComputeSunlightContribution(float3 viewVector, float3 normal, float3 albedo, float4 specular)
{
	float3 sunDir = normalize(SunDirection.xyz);
	
	float diffuseComponent = max(dot(normal, sunDir), 0.1);
	float3 diffuseColor = albedo * diffuseComponent;
	diffuseColor *= SunColor.rgb * SunColor.a;
	
	float3 halfVec = normalize(viewVector + sunDir);
	float specularComponent = max(dot(normal, halfVec), 0.0);
	specularComponent = pow(specularComponent, specular.a);
	float3 specularColor = specular.rgb * specularComponent;
	specularColor *= SunColor.rgb * SunColor.a;
	
	return diffuseColor + specularColor;
}

[[vk::binding(2, 1)]]
Texture2D ShadowTexture : register(t2);
[[vk::binding(2, 2)]]
SamplerState ShadowSampler : register(s2);

[[vk::binding(6, 0)]]
cbuffer ShadowBuffer
 : register(b10){
	float4x4 ShadowMatrix;
};

float SampleShadow(float2 texCoord)
{
	return ShadowTexture.Sample(ShadowSampler, texCoord).r;
}

#define FOG_DENSITY FogControls.x
#define FOG_ENABLED (FogControls.y > 0.0001)

[[vk::binding(4, 0)]]
cbuffer FogBuffer
 : register(b7){
	float4 FogColor;
	float4 FogControls;
};

float3 ComputeFogContribution(float3 inputColor, float distanceToCamera)
{
	if(FOG_ENABLED)
	{
		float fogFactor =  1.0 - clamp(exp(-distanceToCamera * FOG_DENSITY), 0.0, 1.0);
		return lerp(inputColor, FogColor.rgb, fogFactor);
	}
	else
	{
		return inputColor;
	}
}


struct PixelOutput
{
	float4 albedo;
	float4 specular;
	float4 worldPosition;
	float4 normal;
};

float3x3 BuildTBN(float4 normal, float4 tangent)
{
	float3x3 mtx;
	float3 t = tangent.xyz;
	float t2 = dot(t, t);
	t = t2 > 0.0 ? t / sqrt(t2) : float3(0, 0, 0);
	mtx[0] = t;
	mtx[2] = normalize(normal.xyz);
	mtx[1] = cross(mtx[2], mtx[0]);
	return mtx;
}

float4 TransformNormal(float4 bumpNormal, float4 normal, float4 tangent)
{
	return float4(normalize(mul(bumpNormal.xyz * 2.0 - 1.0, BuildTBN(normal, tangent))), 0.0);
}

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 projCoord : PROJCOORD;
	float4 normal : NORMAL;
	float4 position : POSITION;
	float2 texCoord : TEXCOORD;
};

PixelOutput ps(PS_In pIn);

float4 main(PS_In pIn) : SV_TARGET
{
	PixelOutput data = ps(pIn);
	
#ifdef APPLY_ALPHA_TEST
	if(data.albedo.a < 0.0)
		discard;
#endif
	
	float4 outColor = data.albedo;
	
	float3 viewVector = (InvViewMatrix[3] - data.worldPosition).xyz;	
	float distanceToCamera = length(viewVector);
	viewVector /= distanceToCamera;
	
#ifdef APPLY_LIGHTING

	float3 normal = normalize(data.normal.xyz);
	outColor = float4(ComputeSunlightContribution(viewVector, normal, data.albedo.rgb, data.specular), data.albedo.a);
#endif
	
#ifdef APPLY_SHADOWS	
	float4 shadowCoord = mul(data.worldPosition, ShadowMatrix); 
	shadowCoord /= shadowCoord.w;
	if(!(
		shadowCoord.x < -1.0 || shadowCoord.x > 1.0 || 
		shadowCoord.y < -1.0 || shadowCoord.y > 1.0 ||
		shadowCoord.z < 0.0 || shadowCoord.z > 1.0))
	{
		shadowCoord = shadowCoord * 0.5 + 0.5;
		shadowCoord.y = 1.0 - shadowCoord.y;		
		float sDepth = SampleShadow(shadowCoord.xy);	
		
		//shadowCoord.z -= 0.3;
		if(sDepth < shadowCoord.z)
		{
			outColor *= 0.7;
		}
	}
#endif	
	
	
#ifdef APPLY_FOG
	viewVector = (InvViewMatrix[3] - data.worldPosition).xyz;	
	viewVector.y *= 0.1;
	distanceToCamera = length(viewVector);
	outColor.rgb = ComputeFogContribution(outColor.rgb, distanceToCamera);
#endif		

#ifdef RENDER_NORMALS
	outColor = float4(normalize(data.normal.xyz), 1.0);
#endif

#ifdef RENDER_VIEW_VECTOR
	outColor = float4(normalize((InvViewMatrix[3] - data.worldPosition).xyz), 1.0);
#endif

	return outColor;	
}



[[vk::binding(0, 3)]]
Texture2D RippleMap : register(t3);
[[vk::binding(1, 3)]]
Texture2D WaveMap : register(t4);
[[vk::binding(0, 4)]]
SamplerState TextureSampler : register(s3);

#define WATER_RIPPLE_DIR DirParams.xy
#define WATER_RIPPLE_SPEED RippleParams.x
#define WATER_RIPPLE_TILING RippleParams.y
#define WATER_RIPPLE_WARP_SCALE RippleParams.z
 
#define WATER_WAVE_DIR DirParams.zw
#define WATER_WAVE_SPEED WaveParams.x
#define WATER_WAVE_TILING (WaveParams.y * 1)
#define WATER_WAVE_NORM_EQUALIZER (WaveParams.z / 1)
#define WATER_WAVE_ATTEN (WaveParams.w)

[[vk::binding(0, 6)]]
cbuffer MaterialBuffer
 : register(b2){
	float4 DirParams;
	float4 RippleParams;
	float4 WaveParams;
	float4 WaterColor0;
	float4 WaterColor1;
	float4 WaveColor;
	float4 SpecularColor;
};



PixelOutput ps(PS_In pIn)
{
	PixelOutput pOutput;

	float2 texCoord = pIn.position.xz;

	float2 rippleOffset = WATER_RIPPLE_DIR * ELAPSED_TIME * WATER_RIPPLE_SPEED;	
	float2 ripple = 
		WaveMap.Sample(TextureSampler, texCoord * WATER_RIPPLE_TILING + rippleOffset * 2.0).xy + 
		WaveMap.Sample(TextureSampler, -texCoord * WATER_RIPPLE_TILING * 2.0 + rippleOffset).xy;
	ripple *= 0.5;
	ripple = (ripple * 2.0 - 1.0) * WATER_RIPPLE_WARP_SCALE;
	
	float2 waveOffset = WATER_WAVE_DIR * ELAPSED_TIME * WATER_WAVE_SPEED;
	float3 wave0 = WaveMap.Sample(TextureSampler, ripple + texCoord * WATER_WAVE_TILING + waveOffset).rgb;
	float3 wave1 = WaveMap.Sample(TextureSampler, ripple + -texCoord.yx * WATER_WAVE_TILING * 1.25 + waveOffset * 0.75).rgb;
	float3 waveNormal = normalize(((wave0 + wave1) * 0.5).rbg * 2.0 - 1.0);	
	
	pOutput.normal = normalize(mul(normalize(float4((waveNormal * 0.5 + 0.5) * float3(1, WATER_WAVE_NORM_EQUALIZER, 1), 0.0)), NormalMatrix));
		
	float3 V = normalize((InvViewMatrix[3] - pIn.position).xyz);
	float3 N = normalize(mul(float4(waveNormal, 0.0), NormalMatrix)).xyz;
	float cosFresnel = max(dot(V, N), 0.0);
	float fresnel = RippleMap.Sample(TextureSampler, float2(cosFresnel, 0.0)).r;
	//fresnel = pow(1.0- cosFresnel, 2.0);
	
	float2 projCoord = pIn.projCoord.xy / pIn.projCoord.w;
	projCoord = projCoord * 0.5 + 0.5;
	float2 screenPos = projCoord;
	projCoord.y = 1.0 - projCoord.y;
	projCoord += waveNormal.xz * 0.1;
	
	//float fr = max(dot(V, normalize(pIn.normal.xyz)), 0.0);
	//fr = max(dot(V, N2), 0.0);
	//
	//float4 sampledPos = float4(screenPos, SampleDepth(projCoord), 1.0);
	//sampledPos.xy = sampledPos.xy * 2.0 - 1.0;
	//sampledPos = mul(sampledPos, InvProjMatrix);
	//sampledPos /= sampledPos.w;
	//sampledPos = mul(sampledPos, InvViewMatrix);
	//
	///*
	//float4 thisPos = float4(screenPos, pIn.clipPos.z, 1.0);
	//thisPos.xy = thisPos.xy * 2.0 - 1.0;
	//thisPos = mul(thisPos, InvProjMatrix);
	//thisPos /= thisPos.w;
	//thisPos = mul(thisPos, InvViewMatrix);
	//*/	
	//
	////if the sampled position is above the water,
	////reverse the refraction offset. Should fix "most" issues with getting
	////a foreground object incorrectly being refracted in the water
	//if(sampledPos.y > pIn.position.y) 
	//{
	//	projCoord -= waveNormal.xz * 0.1;
	//}
	
	float4 scene = SampleScene(projCoord);

	pOutput.albedo.rgb = lerp(WaterColor0.rgb, WaterColor1.rgb, fresnel * WATER_WAVE_ATTEN);
	//pOutput.albedo.rgb = lerp(pOutput.albedo.rgb, scene.rgb, fr * 0);
	pOutput.worldPosition = pIn.position;
	pOutput.specular = float4(1,1,1, 512);

	return pOutput;
}


