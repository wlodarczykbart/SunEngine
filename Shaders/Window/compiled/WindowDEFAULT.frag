#pragma pack_matrix(row_major)

[[vk::binding(0, 3)]]
Texture2D RainTexture : register(t3);
[[vk::binding(0, 4)]]
SamplerState RainSampler : register(s3);

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


struct PS_In
{
	float4 clipPos : SV_POSITION;	
	float4 projCoord : PROJCOORD;
	float4 position : POSITION;
	float4 normal : NORMAL;
	float2 texCoord : TEXCOORD;
};

[[vk::binding(0, 6)]]
cbuffer MaterialBuffer
 : register(b2){
	float4 DiffuseColor;
	float4 SpecularColor;
	float4 AtlasCoords;
	float4 ObjectCenter;
};

float4 main(PS_In pIn) : SV_TARGET
{
	float4 outColor = float4(0,0,0,1);

	float4 proj = pIn.projCoord / pIn.projCoord.w;
	proj = proj * 0.5 + 0.5;
	proj.y = 1.0 - proj.y;	
	float2 projCoord = proj.xy;	

#ifdef SHADER_PASS_DEFAULT	
	float2 atlasCoord;
	atlasCoord.x = lerp(AtlasCoords.x, AtlasCoords.z, projCoord.x);
	atlasCoord.y = lerp(AtlasCoords.y, AtlasCoords.w, projCoord.y);
	float4 dropColor = RainTexture.Sample(RainSampler, atlasCoord * TextureTransforms[3].xy + TextureTransforms[3].zw);
	
	float dropRefraction = dropColor.r;
	float dropTransparency = dropColor.r;
	
	dropRefraction *= 0.010;	
	projCoord += dropRefraction;
	projCoord.x = clamp(projCoord.x, 0.0, 1.0);
	projCoord.y = clamp(projCoord.y, 0.0, 1.0);
	
	float4 albedo = SampleScene(projCoord);	
	albedo.rgb = lerp(albedo.rgb, float3(1,1,1) * 0.75, dropTransparency * 0.1);	
	//albedo.rgb = lerp(albedo.rgb, DiffuseColor.rgb, DiffuseColor.a);
	albedo.a = 1.0;
	
	float3 viewVector = (InvViewMatrix[3] - pIn.position).xyz;	
	float distanceToCamera = length(viewVector);
	viewVector /= distanceToCamera;
	float3 normal = normalize(pIn.normal.xyz);	
	
	float3 litColor = ComputeSunlightContribution(viewVector, normal, DiffuseColor.rgb, SpecularColor);
	litColor = ComputeFogContribution(litColor, distanceToCamera);
	
	albedo.rgb = lerp(albedo.rgb, litColor, DiffuseColor.a);
	outColor = albedo;
#endif
	
#ifdef SHADER_PASS_STATIC
	
	float tiling = 4.0;
	float rainSpeed = 0.1;
	
	float2 texCoord = pIn.texCoord;
	outColor = float4(texCoord.x, texCoord.y, 0.0, 1.0);
	
	float2 vTime = float2(0.0f, ELAPSED_TIME) * rainSpeed;
	float scrollOffset= RainTexture.Sample(RainSampler, texCoord + vTime * TextureTransforms[3].xy + TextureTransforms[3].zw).g;
	//scrollOffset = pow(scrollOffset, 4.0);	
	
	float dropRefraction = 1.0 - RainTexture.Sample(RainSampler, texCoord * tiling * TextureTransforms[3].xy + TextureTransforms[3].zw).r;
	dropRefraction *= scrollOffset;
	projCoord += dropRefraction;
	projCoord.x = clamp(projCoord.x, 0.0, 1.0);
	projCoord.y = clamp(projCoord.y, 0.0, 1.0);	
	
	//outColor = float4(1,1,1,1) * 1.0 - rainData.x;
	
	outColor = SampleScene(projCoord);
	

	//outColor = float4(1,1,1,1) * scrollOffset;
	
	outColor.a = 1.0;
#endif	
	return outColor;
}


