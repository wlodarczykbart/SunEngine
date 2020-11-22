#pragma pack_matrix(row_major)

[[vk::binding(0, 0)]]
cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};


static float4 VERTS[6] =  
{
	float4(-1, -1, 1, 1),
	float4(+1, -1, 1, 1),
	float4(+1, +1, 1, 1),
	float4(-1, -1, 1, 1),
	float4(+1, +1, 1, 1),
	float4(-1, +1, 1, 1)
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 direction : DIRECTION;
};

PS_In main(uint vertexId : SV_VERTEXID)
{
	PS_In pIn;
	pIn.clipPos = VERTS[vertexId];
	
	float4 dir = pIn.clipPos;
	dir = mul(VERTS[vertexId], InvProjMatrix);
	dir.xyz /= dir.w;
	pIn.direction = mul(float4(dir.xyz, 0.0), InvViewMatrix);
	//pIn.direction.y += InvViewMatrix[3].y; //
	//pIn.direction.y += 50;
	
	return pIn;
}


