#pragma pack_matrix(row_major)

[[vk::binding(0, 0)]]
cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};

[[vk::binding(0, 5)]]
cbuffer ObjectBuffer
 : register(b1){
	float4x4 WorldMatrix;
	float4x4 NormalMatrix;
};


struct VS_In
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 texCoord : TEXCOORD;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 position : POSITION;
	float2 texCoord : TEXCOORD;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	float4 position = mul(vIn.position, WorldMatrix);
	float4 viewPos = mul(position, ViewMatrix);
	pIn.clipPos = mul(viewPos, ProjectionMatrix);
	pIn.texCoord = vIn.texCoord.xy;
	pIn.position = position;
			
	return pIn;
} 


