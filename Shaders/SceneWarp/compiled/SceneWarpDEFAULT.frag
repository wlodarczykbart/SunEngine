#pragma pack_matrix(row_major)

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

#define DELTA_TIME (TimeData.x)
#define ELAPSED_TIME (TimeData.y)
#define HORIZ_WIND WindVec.xz

[[vk::binding(5, 0)]]
cbuffer EnvBuffer
 : register(b9){
	float4 TimeData;
	float4 WindVec;
};

[[vk::binding(0, 7)]]
cbuffer TextureTransformBuffer
 : register(b6){
	float4 TextureTransforms[32];
};

float4 SampleTextureArray(Texture2DArray texArray, SamplerState texArraySampler, float3 texArrayCoord)
{
	float4 transform = TextureTransforms[int(texArrayCoord.z)];
	texArrayCoord.xy = texArrayCoord.xy * transform.xy + transform.zw;
	return texArray.Sample(texArraySampler, texArrayCoord);
}


[[vk::binding(0, 3)]]
Texture2D WarpTexture : register(t3);
[[vk::binding(0, 4)]]
SamplerState WarpSampler : register(s3);

[[vk::binding(0, 6)]]
cbuffer MaterialBuffer
 : register(b2){
	float4 WarpSettings;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;	
	float4 projCoord : PROJCOORD;
	float2 texCoord : TEXCOORD;
};

float4 main(PS_In pIn) : SV_TARGET
{
	float2 projCoord = pIn.projCoord.xy / pIn.projCoord.w;
	projCoord = projCoord * 0.5 + 0.5;
	
	float2 texCoord = pIn.texCoord * 15.0;
	float3 normal0 = WarpTexture.Sample(WarpSampler, texCoord + ELAPSED_TIME * WarpSettings.x * TextureTransforms[3].xy + TextureTransforms[3].zw).rgb;
	float3 normal1 = WarpTexture.Sample(WarpSampler, float2(-texCoord.y, 1.0f - texCoord.x) + ELAPSED_TIME * WarpSettings.x * TextureTransforms[3].xy + TextureTransforms[3].zw).rgb;
	float3 normal = (normal0 + normal1) * 0.5;
	normal = normal * 2.0 - 1.0;
	
	float2 warpCoord = normal.rg * WarpSettings.y;
	normal = WarpTexture.Sample(WarpSampler, texCoord + warpCoord + float2(ELAPSED_TIME, -ELAPSED_TIME) * 0.05 * TextureTransforms[3].xy + TextureTransforms[3].zw).rgb;
	//float3 n2 = WarpTexture.Sample(WarpSampler, texCoord + float2(-warpCoord.y, 1.0 - warpCoord.x) * TextureTransforms[3].xy + TextureTransforms[3].zw).rgb;
	//normal = lerp(normal, n2, 1.0 - (dot(normalize(normal), normalize(n2)) * 0.5 + 0.5));
	warpCoord = (normal.rg * 2.0 - 1.0) * WarpSettings.y;	
	
	projCoord += warpCoord;
	projCoord.x = clamp(projCoord.x, 0.0, 1.0);
	projCoord.y = 1.0 - clamp(projCoord.y, 0.0, 1.0);
	
	float3 baseColor = float3(0.3, 0.7, 0.9);
	float ripple = pow(normal.z, 8.0);
	float4 streakColor = float4(lerp(baseColor, baseColor * 0.5, ripple), 1.0);
	
	float4 sceneCol = lerp(SampleScene(projCoord), streakColor, 0.3);
	
	float4 worldNormal = float4(normal.xzy, 0.0);
	worldNormal = normalize(worldNormal);
	
	float dcol = max(dot(worldNormal, normalize(float4(1.0, -0.5, 1.0, 0.0))), 0.0);
	
	sceneCol = sceneCol ;//* dcol;
	
	return sceneCol;
	
	
}


