#pragma pack_matrix(row_major)
#define APPLY_SHADOWS 1

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 normal : NORMAL;
	float4 position : POSITION;
};




float4 main(PS_In pIn) : SV_TARGET
{
	return float4(1, 1, 1, 1);
}



