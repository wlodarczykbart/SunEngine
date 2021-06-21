Texture2DArray ShadowTexture;
SamplerState ShadowSampler;

cbuffer ShadowBuffer
{
	float4x4 ShadowMatrices[8];
	float4x4 ShadowSplitDepths;
};

uint GetCascadeIndex(float eyeSpaceZ)
{
	[unroll]
	for(uint i = 0; i < CASCADE_SHADOW_MAP_SPLITS; i++)
	{
		float splitDepth = ShadowSplitDepths[i / 4][i % 4];
		if(eyeSpaceZ > splitDepth)
			return i;	
	}	
	return CASCADE_SHADOW_MAP_SPLITS - 1;
}

float SampleShadow(float2 texCoord, int splitIndex)
{
	return ShadowTexture.Sample(ShadowSampler, float3(texCoord, splitIndex)).r;
}

static const float3 testColors[4] = {
	float3(1, 0, 0),
	float3(1, 1, 0),
	float3(1, 0, 1),
	float3(0, 0, 1)
};	
	
//Supports blur size of 1, 3, or 5
static const float2 CascadeShadowMapPCFOffsets[25] = {
	float2(+0.0, +0.0),
	
	float2(+1.0, +0.0),
	float2(-1.0, +0.0),
	float2(+0.0, +1.0),
	float2(+0.0, -1.0),
	
	float2(+1.0, +1.0),
	float2(-1.0, +1.0),
	float2(+1.0, -1.0),
	float2(-1.0, -1.0),
	
	float2(+2.0, +0.0),
	float2(-2.0, +0.0),
	float2(+0.0, +2.0),
	float2(+0.0, -2.0),
	
	float2(+2.0, +1.0),
	float2(-2.0, +1.0),
	float2(+1.0, +2.0),
	float2(+1.0, -2.0),
	
	float2(+2.0, -1.0),
	float2(-2.0, -1.0),
	float2(-1.0, +2.0),
	float2(-1.0, -2.0),	
	
	float2(+2.0, +2.0),
	float2(-2.0, +2.0),
	float2(+2.0, -2.0),
	float2(-2.0, -2.0)
};

float3 ComputeShadowFactor(float4 worldPos, float viewDistance)
{
	uint cascadeIndex = GetCascadeIndex(viewDistance);
	float4 shadowCoord = mul(worldPos, ShadowMatrices[cascadeIndex]);
	shadowCoord /= shadowCoord.w;
	
	float shadowFactor = 1.0;
	float bias = 0.001;
	
	if(shadowCoord.z >-1.0 && shadowCoord.z < 1.0) 
	{
		shadowFactor = 0.0;
		[unroll]
		for(uint i = 0; i < CASCADE_SHADOW_MAP_PCF_BLUR_SIZE_2; i++)
		{
			float depth = SampleShadow(shadowCoord.xy + CascadeShadowMapPCFOffsets[i] * INV_CASCADE_SHADOW_MAP_RESOLUTION, cascadeIndex);
			shadowFactor += depth < shadowCoord.z - bias ? 0.5 : 1.0;
		}
		
		shadowFactor /= CASCADE_SHADOW_MAP_PCF_BLUR_SIZE_2;
	}	

#if 1
	return float3(shadowFactor, shadowFactor, shadowFactor);
#else
	return testColors[cascadeIndex] * shadowFactor;
#endif	
}