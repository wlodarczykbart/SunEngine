#define DELTA_TIME (TimeData.x)
#define ELAPSED_TIME (TimeData.y)
#define HORIZ_WIND WindVec.xz

#define FOG_ENABLED (FogControls.x > 0.5)
#define FOG_HEIGHT_FALLOFF FogControls.y
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

TextureCube EnvTexture;
SamplerState EnvSampler;

float4 SampleEnvironment(float3 dir)
{
	return EnvTexture.Sample(EnvSampler, dir);
};

float3 ComputeFogContribution(float3 inputColor, float3 viewSpacePos, float eyePosY)
{
	if(false)//FOG_ENABLED)
	{
		float cVolFogHeightDensityAtViewer = exp( -FOG_HEIGHT_FALLOFF * eyePosY );	
		float fogInt = length( viewSpacePos ) * cVolFogHeightDensityAtViewer;
		
		const float cSlopeThreshold = 0.01;
		if( abs( viewSpacePos.y ) > cSlopeThreshold )
		{
			float t = FOG_HEIGHT_FALLOFF * viewSpacePos.y;
			fogInt *= ( 1.0 - exp( -t ) ) / t;
		}
		return lerp(inputColor, FogColor.rgb, exp( -FOG_DENSITY * fogInt ));
	}
	else
	{
		return inputColor;
	}
}