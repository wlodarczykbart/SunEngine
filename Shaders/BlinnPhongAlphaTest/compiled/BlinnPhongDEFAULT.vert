#pragma pack_matrix(row_major)
#define APPLY_LIGHTING 1
#define APPLY_FOG 1
#define APPLY_ALPHA_TEST 1

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
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 texCoord : TEXCOORD;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	float4 worldPos = mul(vIn.position, WorldMatrix);
	pIn.clipPos = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
	pIn.position = worldPos;
	pIn.normal = mul(vIn.normal, NormalMatrix);
	pIn.tangent = mul(vIn.tangent, NormalMatrix);
	pIn.texCoord = vIn.texCoord.xy;
			
	return pIn;
}


