#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"
#include "EnvBuffer.hlsl"

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