#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"

struct VS_In
{
	float4 position : POSITION;
	float4 cellData : DATA;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;	
	float4 cellData : DATA;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	pIn.clipPos = mul(mul(mul(vIn.position, WorldMatrix), ViewMatrix), ProjectionMatrix);
	pIn.cellData = vIn.cellData;
	return pIn;
};