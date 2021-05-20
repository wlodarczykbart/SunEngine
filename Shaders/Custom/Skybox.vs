#include "CameraBuffer.hlsl"

static const float3 VERTS[] = 
{
	float3(-1, -1, +1),
	float3(+1, -1, +1),
	float3(+1, +1, +1),
	float3(-1, +1, +1),
	float3(-1, -1, -1),
	float3(+1, -1, -1),
	float3(+1, +1, -1),
	float3(-1, +1, -1)
};

static const uint INDICES[] = 
{
	0, 2, 1,
	0, 3, 2,
	1, 6, 5,
	1, 2, 6,
	5, 7, 4,
	5, 6, 7,
	4, 3, 0,
	4, 7, 3,
	3, 6, 2,
	3, 7, 6,
	4, 1, 5,
	4, 0, 1
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float3 normal : NORMAL;
};

PS_In main(uint vIndex : SV_VERTEXID)
{
	PS_In pIn;
	float3 pos = VERTS[INDICES[vIndex]];
	
	pIn.clipPos = mul(mul(float4(pos, 0.0), ViewMatrix), ProjectionMatrix).xyww;
	pIn.normal = pos;
	
	return pIn;
}