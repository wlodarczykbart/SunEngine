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
	float4 position : POSITION;
	float4 texCoord : TEXCOORD;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	
	pIn.clipPos = mul(mul(mul(vIn.position, WorldMatrix), ViewMatrix), ProjectionMatrix);
	pIn.position = mul(vIn.position, WorldMatrix);
	pIn.texCoord = vIn.texCoord;
	pIn.normal = mul(vIn.normal, NormalMatrix);
	pIn.tangent = mul(vIn.tangent, NormalMatrix);
	return pIn;
}