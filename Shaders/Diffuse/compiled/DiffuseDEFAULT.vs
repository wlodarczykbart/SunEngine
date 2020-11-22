#pragma pack_matrix(row_major)

cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};

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
	float4 normal : NORMAL;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	float4 worldPos = mul(vIn.position, WorldMatrix);
	pIn.clipPos = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
	
	float3 N = mul(vIn.normal, NormalMatrix).xyz;
	pIn.normal = float4(N, 0.0f);	
	
	return pIn;
}


