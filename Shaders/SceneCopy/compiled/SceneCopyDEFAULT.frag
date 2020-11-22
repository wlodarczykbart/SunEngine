#pragma pack_matrix(row_major)

[[vk::binding(0, 0)]]
cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};


[[vk::binding(0, 3)]]
Texture2D SceneCopyTex : register(t3);
[[vk::binding(1, 3)]]
Texture2D DepthCopyTex : register(t4);
[[vk::binding(0, 4)]]
SamplerState Sampler : register(s3);

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

struct PS_Out
{
	float4 color : SV_TARGET;
	float depth : SV_DEPTH;
};

PS_Out main(PS_In pIn)
{
	PS_Out pOut;
	pOut.color = SceneCopyTex.Sample(Sampler, pIn.texCoord);
	pOut.depth = DepthCopyTex.Sample(Sampler, pIn.texCoord).r;
	
	return pOut;
};


