cbuffer TextureTransformBuffer
{
	float4 TextureTransforms[32];
};

float4 SampleTextureArray(Texture2DArray texArray, SamplerState texArraySampler, float3 texArrayCoord)
{
	float4 transform = TextureTransforms[int(texArrayCoord.z)];
	texArrayCoord.xy = texArrayCoord.xy * transform.xy + transform.zw;
	return texArray.Sample(texArraySampler, texArrayCoord);
}