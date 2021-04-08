#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"

struct VS_In
{
	float4 position : POSITION;
	float4 texCoord : TEXCOORD;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
#if !defined(DEPTH) || (defined(DEPTH) && defined(ALPHA_TEST))	
	float4 texCoord : TEXCOORD;	
#endif
#ifndef DEPTH	
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
#endif	
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	
	pIn.clipPos = mul(mul(mul(vIn.position, WorldMatrix), ViewMatrix), ProjectionMatrix);
	
#if !defined(DEPTH) || (defined(DEPTH) && defined(ALPHA_TEST))	
	pIn.texCoord = vIn.texCoord;
#endif
	
#ifndef DEPTH	
	pIn.position = mul(vIn.position, WorldMatrix);
	pIn.normal = mul(float4(vIn.normal.xyz, 0.0), WorldMatrix);
	pIn.tangent = mul(float4(vIn.tangent.xyz, 0.0), WorldMatrix);
	
#if 1
	pIn.position = mul(pIn.position, ViewMatrix);
	pIn.normal = mul(pIn.normal, ViewMatrix);
	pIn.tangent = mul(pIn.tangent, ViewMatrix);
#endif
#endif
	
	return pIn;
}