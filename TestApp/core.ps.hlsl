#pragma pack_matrix(row_major)

#define MAX_CASCADES 8

#define DEPTH_IF_CASCADE(c) case c: depth = DepthTexture##c.Sample(Sampler, shadowCoord.xy).r; 

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 worldPos : WORLDPOSITION;
	float4 eyePos : EYEPOSITION;
	float4 color : COLOR;
};

cbuffer ShadowBuffer : register(b2)
{
	float4x4 ShadowMatrices[MAX_CASCADES];
	float4x4 ShadowSplitDepths;
}

Texture2D DepthTexture0 : register(t0);
Texture2D DepthTexture1 : register(t1);
Texture2D DepthTexture2 : register(t2);
Texture2D DepthTexture3 : register(t3);
Texture2D DepthTexture4 : register(t4);
Texture2D DepthTexture5 : register(t5);
Texture2D DepthTexture6 : register(t6);
Texture2D DepthTexture7 : register(t7);

SamplerState Sampler : register(s0);

//float textureProj(float4 shadowCoord)
//{
//	float shadow = 1.0;
//	float bias = 0.005;
//
//	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
//		//shadowCoord.xy = shadowCoord.xy * 0.5f + 0.5f;
//		//shadowCoord.y = 1.0f - shadowCoord.y;
//		float dist = DepthTexture.Sample(Sampler, shadowCoord.xy).r;
//		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
//			shadow = 0.0f;
//		}
//	}
//	return shadow;
//}
//
//static const float2 ShadowOffsets[9] = { 
//	float2(0, 0), 
//	float2(1, 0), 
//	float2(0, 1), 
//	float2(1, 1), 
//	float2(-1, 0), 
//	float2(0, -1), 
//	float2(-1, -1), 
//	float2(1, -1), 
//	float2(-1, 1) 
//};

static const float4x4 ShadowBiasMatrix = float4x4(
	0.5f, 0.0f, 0.0f, 0.0f,
	0.0f, -0.5f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.0f, 1.0f);

static const uint2 IndexToMatrixIndexTable[MAX_CASCADES] =
{
	uint2(0, 0),
	uint2(0, 1),
	uint2(0, 2),
	uint2(0, 3),
	uint2(1, 0),
	uint2(1, 1),
	uint2(1, 2),
	uint2(1, 3),
};

static const float3 CascadeColors[MAX_CASCADES] =
{
	float3(1, 0, 0),
	float3(0, 1, 0),
	float3(0, 0, 1),
	float3(1, 1, 0),
	float3(1, 0, 1),
	float3(0, 1, 1),
	float3(1, 1, 1) * 0.5,
	float3(1, 1, 1),
};

//static const float TestSplits[MAX_CASCADES] = { 9.97500038*1, 29.9250011, 119.700005, 199.500000, 0.000000000, 0.000000000, 0.000000000, 0.000000000 };

#if 1
#define CASCADE_COLOR(c)	float3(1,1,1)
#else
#define CASCADE_COLOR(c) CascadeColors[c]
#endif

float3 getShadowFactor(float4 eyePos, float4 worldPos)
{
	//float shadowFactor = 0.0;

	//[unroll]
	//for (int i = 0; i < 9; i++)
	//{
	//	shadowFactor += textureProj(shadowCoord + float4(ShadowOffsets[i], 0.0f, 0.0f));
	//}
	//shadowFactor /= 9;
	//return shadowFactor;

	uint cascadeIndex = MAX_CASCADES - 1;
	float eyeDist = eyePos.z;

	[unroll]
	for (uint i = 0; i < MAX_CASCADES; i++)
	{
		if (eyeDist < ShadowSplitDepths[IndexToMatrixIndexTable[i].x][IndexToMatrixIndexTable[i].y])
		{
			cascadeIndex = i;
			break;
		}
	}

	//cascadeIndex = 3;

	float4x4 ShadowMatrix = ShadowMatrices[cascadeIndex];

	float4 shadowCoord = mul(worldPos, ShadowMatrix);
	shadowCoord /= shadowCoord.w;

	float shadowFactor = 1.0f;
	if (
		shadowCoord.x > -1.0f && shadowCoord.x < 1.0f &&
		shadowCoord.y > -1.0f && shadowCoord.y < 1.0f &&
		shadowCoord.z > -1.0f && shadowCoord.z < 1.0f
		)
	{
		const float bias = 0.001;
		shadowCoord = mul(shadowCoord, ShadowBiasMatrix);
		float depth = 1.0f;
		switch (cascadeIndex)
		{
			DEPTH_IF_CASCADE(0); break;
			DEPTH_IF_CASCADE(1); break;
			DEPTH_IF_CASCADE(2); break;
			DEPTH_IF_CASCADE(3); break;
			DEPTH_IF_CASCADE(4); break;
			DEPTH_IF_CASCADE(5); break;
			DEPTH_IF_CASCADE(6); break;
			DEPTH_IF_CASCADE(7); break;
		default:
			break;
		}

		if (depth < shadowCoord.z - bias)
			shadowFactor = 0.0f;
	}

	return shadowFactor * CASCADE_COLOR(cascadeIndex);
}

float4 main(PS_In pIn) : SV_TARGET
{
	return float4(pIn.color.rgb * getShadowFactor(pIn.eyePos, pIn.worldPos), 1.0);
}