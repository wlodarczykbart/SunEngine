#pragma pack_matrix(row_major)

[[vk::binding(0, 6)]]
cbuffer MaterialBuffer
 : register(b2){
	float4 Color;
	float4 BoxMin;
	float4 BoxMax;
};

float4 main() : SV_TARGET
{
	return Color;
}


