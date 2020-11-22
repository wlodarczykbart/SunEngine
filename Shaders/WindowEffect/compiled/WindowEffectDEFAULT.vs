#pragma pack_matrix(row_major)

struct VS_In
{
	float4 position_scale : POSITION;
	float4 data : DATA;
};


struct GS_In
{
	float4 position_scale : POSITION;
	float4 data : DATA;
};

GS_In main(VS_In vIn)
{
	GS_In gIn;
	gIn.position_scale = vIn.position_scale;
	gIn.data = vIn.data;
	return gIn;
}


