//Mapping=SampleDepth

Texture2D DepthTexture;
SamplerState DepthSampler;

float SampleDepth(float2 texCoord)
{
	return DepthTexture.Sample(DepthSampler, texCoord).r;
}