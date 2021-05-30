#pragma pack_matrix(row_major)

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 worldPos : WORLDPOSITION;
	float4 eyePos : EYEPOSITION;
	float4 color : COLOR;
};

cbuffer CameraBuffer : register(b0)
{
	float4x4 ViewProjMatrix;
	float4x4 ViewMatrix;
}

cbuffer ObjectBuffer : register(b1)
{
	float4x4 WorldMatrix;
}

PS_In main(float4 position : POSITION)
{
	PS_In pIn;
	float4 worldPos = mul(position, WorldMatrix);
	pIn.clipPos = mul(worldPos, ViewProjMatrix);
	pIn.worldPos = worldPos;
	pIn.eyePos = mul(worldPos, ViewMatrix);
	pIn.color = position * 0.5 + 0.5;
	return pIn;
};