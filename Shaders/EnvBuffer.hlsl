#define DELTA_TIME (TimeData.x)
#define ELAPSED_TIME (TimeData.y)
#define HORIZ_WIND WindVec.xz

#define FOG_ENABLED (FogControls.x > 0.5)
#define FOG_SAMPLES_SKY (FogControls.y > 0.5)
#define FOG_DENSITY FogControls.z

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

Texture2D SkyTexture;
SamplerState SkySampler;

float3 ComputeFogContribution(float3 inputColor, float distanceToCamera, float2 screenTexCoord)
{
	if(FOG_ENABLED)
	{
		float fogFactor =  1.0 - clamp(exp(-distanceToCamera * FOG_DENSITY), 0.0, 1.0);
		if(FOG_SAMPLES_SKY)
		{
			float4 skyColor = SkyTexture.Sample(SkySampler, screenTexCoord);
			//skyColor.a contains how close the sample is to the horizon, which has a sharp color compared to sky above the horizon, we ues the fog color at this range blending up to the sky color
			//float a = exp(skyColor.a * -300 * 0.001);
			float a = smoothstep(0, 1, skyColor.a);
			a = saturate(pow(skyColor.a, 4.0)*50);
			float3 fogColor = lerp(FogColor.rgb, skyColor.rgb, a);
			//return float3(a,a,a);
			return lerp(inputColor, fogColor, 1*fogFactor);
		}	
		else
		{		
			return lerp(inputColor, FogColor.rgb, fogFactor);
		}
	}
	else
	{
		return inputColor;
	}
}