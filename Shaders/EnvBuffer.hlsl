#define DELTA_TIME (TimeData.x)
#define ELAPSED_TIME (TimeData.y)
#define HORIZ_WIND WindVec.xz

#define FOG_DENSITY FogControls.x
#define FOG_ENABLED (FogControls.y > 0.0001)

cbuffer EnvBuffer
{
	float4 SunDirection;
	float4 SunViewDirection;
	float4 SunColor;

	float4 FogColor;
	float4 FogControls;

	float4 TimeData;
	float4 WindVec;
};

float3 ComputeFogContribution(float3 inputColor, float distanceToCamera)
{
	if(FOG_ENABLED)
	{
		float fogFactor =  1.0 - clamp(exp(-distanceToCamera * FOG_DENSITY), 0.0, 1.0);
		return lerp(inputColor, FogColor.rgb, fogFactor);
	}
	else
	{
		return inputColor;
	}
}