#pragma pack_matrix(row_major)

static const float2 VERTS[] =
{
	float2(-1.0f, -1.0f),
	float2(+1.0f, -1.0f),
	float2(+1.0f, +1.0f),

	float2(-1.0f, -1.0f),
	float2(+1.0f, +1.0f),
	float2(-1.0f, +1.0f),
};

float4 main(uint vIndex : SV_VERTEXID) : SV_POSITION
{
	return float4(VERTS[vIndex], 0.0f, 1.0f);
};