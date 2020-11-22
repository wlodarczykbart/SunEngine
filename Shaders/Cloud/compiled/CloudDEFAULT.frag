#pragma pack_matrix(row_major)

[[vk::binding(0, 7)]]
cbuffer TextureTransformBuffer
 : register(b6){
	float4 TextureTransforms[32];
};

float4 SampleTextureArray(Texture2DArray texArray, SamplerState texArraySampler, float3 texArrayCoord)
{
	float4 transform = TextureTransforms[int(texArrayCoord.z)];
	texArrayCoord.xy = texArrayCoord.xy * transform.xy + transform.zw;
	return texArray.Sample(texArraySampler, texArrayCoord);
}

#define DELTA_TIME (TimeData.x)
#define ELAPSED_TIME (TimeData.y)
#define HORIZ_WIND WindVec.xz

[[vk::binding(5, 0)]]
cbuffer EnvBuffer
 : register(b9){
	float4 TimeData;
	float4 WindVec;
};

[[vk::binding(0, 3)]]
Texture2D CloudMap : register(t3);
[[vk::binding(0, 4)]]
SamplerState CloudSampler : register(s3);

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 position : POSITION;
	float2 texCoord : TEXCOORD;
};

float4 main(PS_In pIn) : SV_TARGET
{
	float4 color = CloudMap.Sample(CloudSampler, (float2(1,1) * ELAPSED_TIME * 0.003) + (pIn.position.xz * 0.0003) * TextureTransforms[3].xy + TextureTransforms[3].zw);
	
	float d = dot(pIn.position.xyz, pIn.position.xyz);
	float t = 1.0 / (d * 0.0000001);
	color.a = min(t, 1.0);
	color.rgb *= 0.2;
	return color;
	
}

