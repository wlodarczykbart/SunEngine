Texture2D ShadowTexture;
SamplerState ShadowSampler;

cbuffer ShadowBuffer
{
	float4x4 ShadowMatrix;
};

float SampleShadow(float2 texCoord)
{
	return ShadowTexture.Sample(ShadowSampler, texCoord).r;
}