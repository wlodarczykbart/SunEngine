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
	float4 position : POSITION;	
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 texCoord : TEXCOORD;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	float4 worldPos = mul(vIn.position, WorldMatrix);
	pIn.clipPos = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
	pIn.position = worldPos;
	pIn.normal = mul(vIn.normal, NormalMatrix);
	pIn.tangent = mul(vIn.tangent, NormalMatrix);
	pIn.texCoord = vIn.texCoord.xy;
			
	return pIn;
}