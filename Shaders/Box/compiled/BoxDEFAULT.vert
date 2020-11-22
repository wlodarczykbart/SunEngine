#pragma pack_matrix(row_major)

[[vk::binding(0, 0)]]
cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};

[[vk::binding(0, 5)]]
cbuffer ObjectBuffer
 : register(b1){
	float4x4 WorldMatrix;
	float4x4 NormalMatrix;
};


[[vk::binding(0, 6)]]
cbuffer MaterialBuffer
 : register(b2){
	float4 Color;
	float4 BoxMin;
	float4 BoxMax;
};

static int IndexTable[36] = 
{
	0, 1, 2,
	0, 2, 3,
	
	1, 4, 7,
	1, 7, 2,
	
	4, 5, 6,
	4, 6, 7,
	
	5, 0, 3,
	5, 3, 6,
	
	3, 2, 7,
	3, 7, 6,
	
	5, 4, 1,
	5, 1, 0
};

float4 main(uint vertexId : SV_VERTEXID) : SV_POSITION
{
	float4 position = float4(0, 0, 0, 1);
	int idx = IndexTable[vertexId];
	
	if(idx == 0)
	{
		position = float4(BoxMin.x, BoxMin.y, BoxMax.z, 1.0);
	}
	else if(idx == 1)
	{
		position = float4(BoxMax.x, BoxMin.y, BoxMax.z, 1.0);	
	}
	else if(idx == 2)
	{
		position = float4(BoxMax.x, BoxMax.y, BoxMax.z, 1.0);
	}
	else if(idx == 3)
	{
		position = float4(BoxMin.x, BoxMax.y, BoxMax.z, 1.0);	
	}
	else if(idx == 4)
	{
		position = float4(BoxMax.x, BoxMin.y, BoxMin.z, 1.0);	
	}
	else if(idx == 5)
	{
		position = float4(BoxMin.x, BoxMin.y, BoxMin.z, 1.0);
	}
	else if(idx == 6)
	{
		position = float4(BoxMin.x, BoxMax.y, BoxMin.z, 1.0);	
	}
	else if(idx == 7)
	{
		position = float4(BoxMax.x, BoxMax.y, BoxMin.z, 1.0);
	}	

	float4 worldPos = mul(float4(position.xyz, 1.0), WorldMatrix);
	return mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
}


