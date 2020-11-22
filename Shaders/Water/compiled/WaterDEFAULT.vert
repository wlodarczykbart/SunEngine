#pragma pack_matrix(row_major)
#define APPLY_LIGHTING 1
#define APPLY_FOG 1

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
	float4 projCoord : PROJCOORD;
	float4 normal : NORMAL;
	float4 position : POSITION;
	float2 texCoord : TEXCOORD;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	float4 worldPos = mul(float4(vIn.position.xyz * float3(1,0,1), 1.0), WorldMatrix);
	pIn.clipPos = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
	pIn.projCoord = pIn.clipPos;
	pIn.normal = mul(float4(vIn.normal.xyz, 0.0), NormalMatrix);
	pIn.position = worldPos;
	pIn.texCoord = vIn.texCoord;
	return pIn;
}


