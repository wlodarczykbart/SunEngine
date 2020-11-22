#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"

#define PARTICLE_POS gIn[0].position
#define PARTICLE_SIZE gIn[0].data.x


struct GS_In
{
	float4 position : POSITION;
	float4 data : PDATA;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 data : PDATA;
	float2 texCoord : TEXCOORD;
};

[maxvertexcount(4)]
void main(point GS_In gIn[1], inout TriangleStream<PS_In> pStream)
{
	PS_In pIn;
	float4 worldPos = mul(PARTICLE_POS, WorldMatrix);
	
	float4 quadPoints[4];
	quadPoints[0] = float4(PARTICLE_SIZE, -PARTICLE_SIZE, 0.0, 0.0);
	quadPoints[1] = float4(PARTICLE_SIZE, PARTICLE_SIZE, 0.0, 0.0);
	quadPoints[2] = float4(-PARTICLE_SIZE, -PARTICLE_SIZE, 0.0, 0.0);
	quadPoints[3] = float4(-PARTICLE_SIZE, PARTICLE_SIZE, 0.0, 0.0);
	
	[unroll]
	for(int i = 0; i < 4; i++)
	{
		pIn.data = gIn[0].data;	
		pIn.texCoord = (quadPoints[i].xy / PARTICLE_SIZE) * 0.5 + 0.5;
		float4 quadPosition = mul(quadPoints[i], InvViewMatrix) + worldPos;
		pIn.clipPos = mul(mul(quadPosition, ViewMatrix), ProjectionMatrix);
		pStream.Append(pIn);
	}	

}