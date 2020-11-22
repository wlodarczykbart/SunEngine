#pragma pack_matrix(row_major)
#define APPLY_FOG 1

cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};


struct VS_In
{
	float4 dummyPosition : POSITION;
};

//Indices should be layed out in this order...
//0, 1, 2,
//0, 2, 3,
//1, 5, 6,
//1, 6, 2,
//5, 4, 7,
//5, 7, 6,
//4, 0, 3,
//4, 3, 7,
//3, 2, 6,
//3, 6, 7,
//4, 5, 1,
//4, 1, 0

static float4 cubePositions[] = 
{
	float4(-1, -1, +1, 1.0f),
	float4(+1, -1, +1, 1.0f),
	float4(+1, +1, +1, 1.0f),
	float4(-1, +1, +1, 1.0f),
	                                                            
	float4(-1, -1, -1, 1.0f),
	float4(+1, -1, -1, 1.0f),
	float4(+1, +1, -1, 1.0f),
	float4(-1, +1, -1, 1.0f)
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 skyPos : POSITION;
};

PS_In main(VS_In vIn, uint vertexIndex : SV_VERTEXID)
{
	PS_In pIn;
	
	float4 position = cubePositions[vertexIndex];
	float4 viewPos = mul(float4(position.xyz, 0.0), ViewMatrix);
	//viewPos.w = 1.0;
	pIn.clipPos = mul(viewPos, ProjectionMatrix).xyww;
	pIn.skyPos = position;
	
	return pIn;
}


