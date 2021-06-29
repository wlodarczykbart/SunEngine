#include "ObjectBuffer.hlsl"
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
	float4 texCoord : TEXCOORD;	
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	
	float4 worldPos = mul(vIn.position, WorldMatrix);
	pIn.clipPos = mul(worldPos, ViewProjectionMatrix);
	pIn.texCoord = vIn.texCoord;
	
	pIn.position = worldPos;	
	pIn.normal = mul(float4(vIn.normal.xyz, 0.0), WorldMatrix);
	pIn.tangent = mul(float4(vIn.tangent.xyz, 0.0), WorldMatrix);	
	
	return pIn;
}