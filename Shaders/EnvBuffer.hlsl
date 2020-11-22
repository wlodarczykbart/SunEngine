#define DELTA_TIME (TimeData.x)
#define ELAPSED_TIME (TimeData.y)
#define HORIZ_WIND WindVec.xz

cbuffer EnvBuffer
{
	float4 TimeData;
	float4 WindVec;
};