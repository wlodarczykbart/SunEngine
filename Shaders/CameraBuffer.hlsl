cbuffer CameraBuffer
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
	float4 Viewport;
};

float2 GetScreenTexCoord(float2 screenPosition)
{
	return (screenPosition - Viewport.xy) / Viewport.zw;
}