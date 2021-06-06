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
#endif	
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
#ifndef DEPTH		
	pIn.position = mul(mul(vIn.position, WorldMatrix), ViewMatrix);
	pIn.normal = mul(mul(float4(vIn.normal.xyz, 0.0), WorldMatrix), ViewMatrix);
	pIn.clipPos  = mul(pIn.position, ProjectionMatrix);	
#else
	pIn.clipPos = mul(mul(vIn.position, WorldMatrix), ViewProjectionMatrix);
#endif
	return pIn;
}