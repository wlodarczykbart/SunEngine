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
	float3 normal : NORMAL;
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	float3 pos = vIn.position.xyz * 2.0; //assumes a unit cube is input which has radius of 0.5, need to double it
	
	pIn.clipPos = mul(float4(pos, 0.0), ViewProjectionMatrix).xyww;
	pIn.normal = pos;
	
	return pIn;
}