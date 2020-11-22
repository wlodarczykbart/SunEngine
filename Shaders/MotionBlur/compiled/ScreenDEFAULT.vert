#pragma pack_matrix(row_major)

static float4 VERTS[6] =  
{
	float4(-1, -1, 0, 1),
	float4(+1, -1, 0, 1),
	float4(+1, +1, 0, 1),
	float4(-1, -1, 0, 1),
	float4(+1, +1, 0, 1),
	float4(-1, +1, 0, 1)
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

PS_In main(uint vertexId : SV_VERTEXID)
{
	PS_In pIn;
	pIn.clipPos = VERTS[vertexId];
	pIn.texCoord = pIn.clipPos.xy * 0.5 + 0.5;
	pIn.texCoord.y = 1.0 - pIn.texCoord.y;
	
	return pIn;
}


