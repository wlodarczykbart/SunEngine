#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"

struct VS_In
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 texCoord : TEXCOORD;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
	float4 normal : NORMAL;
	float4 position : POSITION;	
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	float4 worldPos = mul(vIn.position, WorldMatrix);
	pIn.clipPos = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
	pIn.normal = normalize(mul(vIn.normal, WorldMatrix));
	pIn.position = worldPos;
	pIn.texCoord = vIn.texCoord.xy;
	return pIn;
}