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

#define DELTA_TIME (TimeData.x)
#define ELAPSED_TIME (TimeData.y)
#define HORIZ_WIND WindVec.xz

[[vk::binding(5, 0)]]
cbuffer EnvBuffer
 : register(b9){
	float4 TimeData;
	float4 WindVec;
};


struct VS_In
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 texCoord : TEXCOORD;
	float4 animData : ANIMDATA;
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
	float4 pos = vIn.position;
	pos.x += cos(ELAPSED_TIME * vIn.animData.y) * vIn.animData.z * vIn.animData.x;
	pos.z += sin(ELAPSED_TIME * vIn.animData.y) * vIn.animData.z * vIn.animData.x;

	PS_In pIn;
	float4 worldPos = mul(pos, WorldMatrix);
	pIn.clipPos = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
	pIn.position = worldPos;
	pIn.texCoord = vIn.texCoord.xy;
	
#ifdef ANIM_AS_NORMAL
	float3 N = normalize(float3(animData.y, animData.z, 0.0)) * 0.5 + float3(0.5, 0.5, 0.5);
	pIn.normal = float4(N, 0.0);
	pIn.tangent = float4(0, 0, 0, 0);
#else
	pIn.normal = mul(vIn.normal, NormalMatrix);
	pIn.tangent = mul(vIn.tangent, NormalMatrix);	
#endif	
	
	return pIn;
}


