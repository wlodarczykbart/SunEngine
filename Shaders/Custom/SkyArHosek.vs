#include "CameraBuffer.hlsl"

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
	float2 texCoord : TEXCOORD;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	pIn.clipPos = float4(vIn.position.xyz * 2.0, 1.0);
	pIn.texCoord = float2(vIn.texCoord.x, 1.0 - vIn.texCoord.y);
	
	return pIn;
}