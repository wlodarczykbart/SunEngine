#include "EnvBuffer.hlsl"

RWTexture2D<float4> DiffuseMap;

#define PI 3.14159


[numthreads(16, 16, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
	uint width;
	uint height;
	DiffuseMap.GetDimensions(width, height);

	float2 uv;
	uv.x = float(threadID.x) / float(width);
	uv.y =  float(threadID.y) / float(height);

	float frequency = 0.5;
	float phi = sin(threadID.x * ELAPSED_TIME * frequency);
	float theta = sin(threadID.y * ELAPSED_TIME * frequency);

	DiffuseMap[int2(threadID.xy)] = float4(uv, phi*theta, 1);
}