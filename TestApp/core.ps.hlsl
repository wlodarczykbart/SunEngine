#pragma pack_matrix(row_major)

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 position : POSITION;
	float4 color : COLOR;
};

cbuffer MatrixBuffer : register(b0)
{
	float4x4 ViewProjMatrix;
	float4x4 WorldMatrix;
	float4x4 ShadowMatrix;
};

Texture2D DepthTexture;
SamplerState Sampler;

float textureProj(float4 shadowCoord)
{
	float shadow = 1.0;
	float bias = 0.005;

	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
		//shadowCoord.xy = shadowCoord.xy * 0.5f + 0.5f;
		//shadowCoord.y = 1.0f - shadowCoord.y;
		float dist = DepthTexture.Sample(Sampler, shadowCoord.xy).r;
		if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
			shadow = 0.0f;
		}
	}
	return shadow;
}

static const float2 ShadowOffsets[9] = { 
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

float getShadowFactor(float4 shadowCoord)
{
	float shadowFactor = 0.0;

	[unroll]
	for (int i = 0; i < 9; i++)
	{
		shadowFactor += textureProj(shadowCoord + float4(ShadowOffsets[i], 0.0f, 0.0f));
	}
	shadowFactor /= 9;
	return shadowFactor;
}

float4 main(PS_In pIn) : SV_TARGET
{
	float4 shadowCoord = mul(pIn.position, ShadowMatrix);
	shadowCoord /= shadowCoord.w;

	return float4(pIn.color.rgb * getShadowFactor(shadowCoord), 1.0);
}