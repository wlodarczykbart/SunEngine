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
	float4 normal : NORMAL;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	pIn.clipPos  = mul(mul(vIn.position, WorldMatrix), ViewProjectionMatrix);	
	pIn.normal = vIn.normal;
	return pIn;
}