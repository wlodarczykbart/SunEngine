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
	float4 normal : NORMAL;
	float4 position : POSITION;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	float4 worldPos = mul(float4(vIn.position.xyz, 1.0), WorldMatrix);
	pIn.clipPos = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
	pIn.normal = normalize(mul(vIn.normal, WorldMatrix));
	pIn.position = worldPos;
	return pIn;
}
