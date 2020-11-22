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
	float4 texCoord : TEXCOORD;
	float4 color : COLOR;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 color : COLOR;
	float2 texCoord : TEXCOORD;
};

PS_In main(VS_In vIn) 
{
	PS_In pIn;
	pIn.clipPos = mul(mul(mul(vIn.position, WorldMatrix), ViewMatrix), ProjectionMatrix);
	pIn.texCoord = vIn.texCoord.xy;
	pIn.color = vIn.color;
	
	return pIn;
}


