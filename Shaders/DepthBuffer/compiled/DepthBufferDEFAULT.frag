#pragma pack_matrix(row_major)

[[vk::binding(0, 3)]]
Texture2D Texture : register(t3);
[[vk::binding(0, 4)]]
SamplerState TextureSampler : register(s3);

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 color : COLOR;
	float2 texCoord : TEXCOORD;
};

float4 main(PS_In pIn) : SV_TARGET
{
	float depth = Texture.Sample(TextureSampler, pIn.texCoord).r;
	
	depth = pow(depth, 32.0);
	
	return float4(depth, depth, depth, 1.0);
}


