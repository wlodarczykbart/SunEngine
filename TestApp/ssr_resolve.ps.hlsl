#pragma pack_matrix(row_major)

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

Texture2D ColorTexture : register(t0);
Texture2D ReflectionColorTexture : register(t1);
SamplerState Sampler : register(s0);

float4 main(PS_In pIn) : SV_TARGET
{
	//float2 invTexSize;
	//ColorTexture.GetDimensions(invTexSize.x, invTexSize.y);
	//invTexSize = 1.0f / invTexSize;

	//float2 texCoord = clipPos.xy * invTexSize;

	const int samples = 2;

	float4 baseColor = ColorTexture.Sample(Sampler, pIn.texCoord);

	float4 reflectionColor = float4(0, 0, 0, 0);

	float numSamples = 0.0f;

	float2 invTexSize;
	ReflectionColorTexture.GetDimensions(invTexSize.x, invTexSize.y);
	invTexSize = 1.0f / invTexSize;

	[unroll]
	for (int i = -samples; i <= samples; i++)
	{
		[unroll]
		for (int j = -samples; j <= samples; j++)
		{
			float4 refColor = ReflectionColorTexture.Sample(Sampler, pIn.texCoord + float2(j, i) * invTexSize);
			reflectionColor += refColor;
			numSamples += 1.0f;
		}
	}

	reflectionColor /= numSamples;

	//reflectionColor = ReflectionColorTexture.Sample(Sampler, pIn.texCoord);

	float3 outColor = lerp(baseColor.rgb, reflectionColor.rgb, reflectionColor.a*0.25);
	return float4(outColor, 1.0);
}
