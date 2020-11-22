#pragma pack_matrix(row_major)

struct VS_In
{
	float4 position : POSITION;
	float4 data : PDATA;
};

struct GS_In
{
	float4 position : POSITION;
	float4 data : PDATA;
};

GS_In main(VS_In vIn)
{
	GS_In gIn;
	gIn.position = vIn.position;
	gIn.data = vIn.data;
	return gIn;
}


