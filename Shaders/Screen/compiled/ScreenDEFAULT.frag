#pragma pack_matrix(row_major)

[[vk::binding(0, 3)]]
Texture2D ScreenTex : register(t3);
[[vk::binding(0, 4)]]
SamplerState ScreenSampler : register(s3);

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

float4 main(PS_In pIn) : SV_TARGET
{
	return float4(ScreenTex.Sample(ScreenSampler, pIn.texCoord).rgb, 1.0);
};


