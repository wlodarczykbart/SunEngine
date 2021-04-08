Texture2D ShadowTexture;
SamplerState ShadowSampler;

cbuffer ShadowBuffer
{
	float4x4 ShadowMatrices[MAX_SHADOW_CASCADE_SPLITS];
};

float SampleShadow(float2 texCoord)
{
	return ShadowTexture.Sample(ShadowSampler, texCoord).r;
}