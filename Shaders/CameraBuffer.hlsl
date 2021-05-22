cbuffer CameraBuffer
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatfrix;
	float4x4 ViewProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjectionMatrix;
	float4x4 InvViewProjectionMatrix;
	float4 Viewport;
};

float2 GetScreenTexCoord(float2 screenPosition)
{
	return (screenPosition - Viewport.xy) / Viewport.zw;
}