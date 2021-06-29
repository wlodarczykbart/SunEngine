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

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

PS_In main(uint vIndex : SV_VERTEXID)
{
	PS_In pIn;
	pIn.clipPos = float4(VERTS[vIndex], 0.0f, 1.0f);
	pIn.texCoord = VERTS[vIndex] * 0.5 + 0.5;
	pIn.texCoord.y = 1.0f - pIn.texCoord.y;
	return pIn;
};