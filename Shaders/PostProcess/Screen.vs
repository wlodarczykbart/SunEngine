struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

static const float2 VERTS[] = 
{
	float2(-1, -1),
	float2(+1, -1),
	float2(+1, +1),
		
	float2(-1, -1),
	float2(+1, +1),
	float2(-1, +1)
};

PS_In main(uint vIndex : SV_VERTEXID)
{
	PS_In pIn;
	pIn.clipPos = float4(VERTS[vIndex], 0.0, 1.0);
	pIn.texCoord = VERTS[vIndex] * 0.5 + 0.5;
	pIn.texCoord.y = 1.0 - pIn.texCoord.y;
	return pIn;
}