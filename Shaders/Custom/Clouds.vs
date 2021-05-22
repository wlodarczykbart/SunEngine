#include "CameraBuffer.hlsl"

static const float3 VERTS[] = 
{
	float3(-1, 0, +1),
	float3(+1, 0, +1),
	float3(+1, 0, -1),
	float3(-1, 0, -1),
};

static const uint INDICES[] = 
{
	0, 2, 1,
	0, 3, 2,
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 gridPos : POSITION;
	float2 texCoord : TEXCOORD;
};

PS_In main(uint vIndex : SV_VERTEXID)
{
	PS_In pIn;
	float3 pos = VERTS[INDICES[vIndex]];
	pIn.texCoord = pos.xz * 0.5 + 0.5;
	
	pos *= 1000.0;
	pIn.gridPos = pos.xz;
	
	pos += InvViewMatrix[3].xyz;
	pos.y += 160.0;
	
	pIn.clipPos  = mul(float4(pos, 1.0), ViewProjectionMatrix);
	
	return pIn;
}