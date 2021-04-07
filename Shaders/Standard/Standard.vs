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
	pIn.normal = mul(float4(vIn.normal.xyz, 0.0), WorldMatrix);
	pIn.tangent = mul(float4(vIn.tangent.xyz, 0.0), WorldMatrix);
	
#if 1
	pIn.position = mul(pIn.position, ViewMatrix);
	pIn.normal = mul(pIn.normal, ViewMatrix);
	pIn.tangent = mul(pIn.tangent, ViewMatrix);
#endif
	
	return pIn;
}