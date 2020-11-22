#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"

struct VS_In
{
	float4 position : POSITION;
	float4 color : COLOR;
	float4 texCoord : TEXCOORD;
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