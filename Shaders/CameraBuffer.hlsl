#define VIEWPORT CameraData[0]

cbuffer CameraBuffer
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 ViewProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjectionMatrix;
	float4x4 InvViewProjectionMatrix;
	float4x4 CameraData;
};

float2 GetScreenTexCoord(float2 screenPosition)
{
	return (screenPosition - VIEWPORT.xy) / VIEWPORT.zw;
}