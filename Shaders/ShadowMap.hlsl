Texture2D ShadowTexture;
SamplerState ShadowSampler;

cbuffer ShadowBuffer
{
	float4x4 ShadowMatrices[8];
	float4x4 ShadowSplitDepths;
};

uint GetCascadeIndex(float eyeSpaceZ)
{
	[unroll]
	for(uint i = 0; i < MAX_SHADOW_CASCADE_SPLITS; i++)
	{
		float splitDepth = ShadowSplitDepths[i / 4][i % 4];
		if(eyeSpaceZ > splitDepth)
			return i;	
	}	
	return MAX_SHADOW_CASCADE_SPLITS - 1;
}

float SampleShadow(float2 texCoord, int splitIndex)
{
	float rangeStart = splitIndex * INV_MAX_SHADOW_CASCADE_SPLITS;
	texCoord.x = lerp(rangeStart, rangeStart + INV_MAX_SHADOW_CASCADE_SPLITS, saturate(texCoord.x));
	return ShadowTexture.Sample(ShadowSampler, texCoord).r;
}

static const float3 testColors[4] = {
	float3(1, 0, 0),
	float3(1, 1, 0),
	float3(1, 0, 1),
	float3(0, 0, 1)
};	
	
static const float2 PCFOffsets[9] = {
	float2(0.0, 0.0),
	float2(INV_CASCADE_SHADOW_MAP_RESOLUTION, 0.0),
	float2(0.0, INV_CASCADE_SHADOW_MAP_RESOLUTION),
	float2(INV_CASCADE_SHADOW_MAP_RESOLUTION, INV_CASCADE_SHADOW_MAP_RESOLUTION),
	float2(-INV_CASCADE_SHADOW_MAP_RESOLUTION, 0.0),
	float2(0.0, -INV_CASCADE_SHADOW_MAP_RESOLUTION),
	float2(-INV_CASCADE_SHADOW_MAP_RESOLUTION, INV_CASCADE_SHADOW_MAP_RESOLUTION),
	float2(INV_CASCADE_SHADOW_MAP_RESOLUTION, -INV_CASCADE_SHADOW_MAP_RESOLUTION),
	float2(-INV_CASCADE_SHADOW_MAP_RESOLUTION, -INV_CASCADE_SHADOW_MAP_RESOLUTION)
};

#define PCF_SAMPLES 9
	
float3 ComputeShadowFactor(float3 viewPos)
{
	uint cascadeIndex = GetCascadeIndex(viewPos.z);

	float4 worldPos = mul(float4(viewPos, 1.0), InvViewMatrix);
	float4 shadowCoord = mul(worldPos, ShadowMatrices[cascadeIndex]);
	shadowCoord /= shadowCoord.w;
	
	float shadowFactor = PCF_SAMPLES;
	float bias = 0.001;
	
	if(
		shadowCoord.x > 0.0 && 
		shadowCoord.x < 1.0 && 
		shadowCoord.y > 0.0 && 
		shadowCoord.y < 1.0 && 
		shadowCoord.z >-1.0 && 
		shadowCoord.z < 1.0) 
	{
		[unroll]
		for(uint i = 0; i < PCF_SAMPLES; i++)
		{
			float depth = SampleShadow(shadowCoord.xy + PCFOffsets[i], cascadeIndex);
			if(depth < shadowCoord.z - bias) shadowFactor -= 1.0;
		}
	}	
	
	shadowFactor /= PCF_SAMPLES;
#if 1
	return float3(shadowFactor, shadowFactor, shadowFactor);
#else
	return testColors[cascadeIndex] * shadowFactor;
#endif	
}