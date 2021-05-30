#pragma pack_matrix(row_major)

#define MAX_CASCADES 8

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 worldPos : WORLDPOSITION;
	float4 eyePos : EYEPOSITION;
	float4 color : COLOR;
};

#define SHADOW_SLICE_RATIO ShadowSplitDepths[3][2]
#define INV_SHADOW_MAP_RESOLUTION ShadowSplitDepths[3][3]

cbuffer ShadowBuffer : register(b2)
{
	float4x4 ShadowMatrices[MAX_CASCADES];
	float4x4 ShadowSplitDepths;
}

Texture2D DepthTexture;
SamplerState Sampler : register(s0);

static const float2 PCFOffsets[9] = { 
	float2(0, 0), 
	float2(1, 0), 
	float2(0, 1), 
	float2(1, 1), 
	float2(-1, 0), 
	float2(0, -1), 
	float2(-1, -1), 
	float2(1, -1), 
	float2(-1, 1) 
};

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
	float3(1, 0, 1),
	float3(1, 1, 0),
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

float textureProj(float4 shadowCoord, uint cascadeIndex)
{
	const float bias = 0.002;

	shadowCoord.x = lerp(cascadeIndex * SHADOW_SLICE_RATIO, (1 + cascadeIndex) * SHADOW_SLICE_RATIO, shadowCoord.x);
	float depth = DepthTexture.Sample(Sampler, shadowCoord.xy).r;
	return depth < shadowCoord.z - bias ? 0.25f : 1.0f;
}

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

	float4 shadowCoord = mul(worldPos, ShadowMatrices[cascadeIndex]);
	shadowCoord /= shadowCoord.w;

	float shadowFactor = 1.0f;
	if (
		shadowCoord.x > -1.0f && shadowCoord.x < 1.0f &&
		shadowCoord.y > -1.0f && shadowCoord.y < 1.0f &&
		shadowCoord.z > -1.0f && shadowCoord.z < 1.0f
		)
	{
		shadowCoord = mul(shadowCoord, ShadowBiasMatrix);

		shadowFactor = 0.0f;
		[unroll]
		for (uint i = 0; i < 9; i++)
		{
			shadowFactor += textureProj(shadowCoord + float4(PCFOffsets[i] * INV_SHADOW_MAP_RESOLUTION, 0.0f, 0.0f), cascadeIndex);
		}
		shadowFactor /= 9.0f;
	}

	return shadowFactor * CASCADE_COLOR(cascadeIndex);
}

float4 main(PS_In pIn) : SV_TARGET
{
	return float4(pIn.color.rgb * getShadowFactor(pIn.eyePos, pIn.worldPos), 1.0);
}