Texture2D SceneTexture;
SamplerState SceneSampler;
Texture2D DepthTexture;
SamplerState DepthSampler;

cbuffer SampleSceneBuffer
{
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