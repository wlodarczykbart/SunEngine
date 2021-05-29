#pragma pack_matrix(row_major)

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 position : POSITION;
	float4 color : COLOR;
};

cbuffer MatrixBuffer : register(b0)
{
	float4x4 ViewProjMatrix;
	float4x4 WorldMatrix;
	float4x4 ShadowMatrix;
};

PS_In main(float4 position : POSITION)
{
	PS_In pIn;
	pIn.position = mul(position, WorldMatrix);
	pIn.clipPos = mul(pIn.position, ViewProjMatrix);
	pIn.color = position * 0.5 + 0.5;
	return pIn;
};