#include "ObjectBuffer.hlsl"
#include "CameraBuffer.hlsl"

struct VS_In
{
	float4 position : POSITION;
	float4 normal : NORMAL;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
#ifndef DEPTH		
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 texCoord : TEXCOORD;
#endif	
};

cbuffer MaterialBuffer
{
	float4 PosToUV;
	float4 TextureTiling[MAX_TERRAIN_SPLAT_MAPS];
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
#ifndef DEPTH		
	pIn.position = mul(mul(vIn.position, WorldMatrix), ViewMatrix);
	pIn.normal = mul(mul(float4(vIn.normal.xyz, 0.0), WorldMatrix), ViewMatrix);
	pIn.tangent = mul(mul(float4(vIn.normal.yxz * float3(1,-1,1), 0.0), WorldMatrix), ViewMatrix);
	pIn.clipPos  = mul(pIn.position, ProjectionMatrix);	
	pIn.texCoord = vIn.position.xz * PosToUV.xy + PosToUV.zw;
#else
	pIn.clipPos = mul(mul(vIn.position, WorldMatrix), ViewProjectionMatrix);
#endif
	return pIn;
}