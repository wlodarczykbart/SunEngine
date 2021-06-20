#include "CameraBuffer.hlsl"

static const float2 VERTS[6] =  
{
	float2(-1, -1),
	float2(+1, -1),
	float2(+1, +1),
	float2(-1, -1),
	float2(+1, +1),
	float2(-1, +1)
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float3 direction : DIRECTION;
};

PS_In main(uint vIndex : SV_VERTEXID)
{
	PS_In pIn;
	
#ifndef ONE_Z
	pIn.clipPos = float4(VERTS[vIndex], 0.0, 1.0);
#else	
	pIn.clipPos = float4(VERTS[vIndex], 1.0, 1.0);
#endif	
	
	float4 viewPos = mul(float4(VERTS[vIndex], 0.0, 1.0), InvProjectionMatrix);
	viewPos /= viewPos.w;	
	viewPos = mul(float4(viewPos.xyz, 0.0), InvViewMatrix);
	pIn.direction = viewPos;
	
	return pIn;
}