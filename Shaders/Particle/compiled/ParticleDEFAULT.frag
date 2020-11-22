#pragma pack_matrix(row_major)

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 data : PDATA;
	float2 texCoord : TEXCOORD;
};


float4 main(PS_In pIn) : SV_TARGET
{
	float roundingFactor = min(length(pIn.texCoord * 2.0 - 1.0), 1.0);
	//roundingFactor = roundingFactor * roundingFactor;

	return float4(pIn.texCoord.x, pIn.texCoord.y, 0.0, 1.0 - roundingFactor);
}


