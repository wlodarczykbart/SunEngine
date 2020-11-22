#pragma pack_matrix(row_major)

#define COLOR(coord) Texture.Sample(Sampler, coord)
#define COLOR_OFFSET(coord, offset) Texture.Sample(Sampler, (coord + (offset * SizeData.xy)))

#define LUMA(col) sqrt(dot(col.rgb, float3(0.299, 0.587, 0.114)))
#define ONE float4(1,1,1,1)

#define MATERIAL
#define ITERATIONS 12

#define INV_WIDTH SizeData.x
#define INV_HEIGHT SizeData.y
#define MIN_LUMA Params.x
#define MAX_THRESHOLD Params.y
#define SUBPIXEL_QUALITY Params.z

[[vk::binding(0, 6)]]
MATERIAL cbuffer MaterialBuffer
 : register(b2){
	float4 SizeData;
	float4 Params;
};

static float gQuality[12] = 
{
	1, 1, 1, 1, 1, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0
};

[[vk::binding(0, 3)]]
Texture2D Texture : register(t3);
[[vk::binding(0, 4)]]
SamplerState Sampler : register(s3);

struct VS_Out
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

float4 main(VS_Out pIn) : SV_TARGET
{
	float2 uv = pIn.texCoord;

	float4 center = COLOR(uv);
	float4 left = COLOR_OFFSET(uv, float2(-1, 0));
	float4 right = COLOR_OFFSET(uv, float2(1, 0));
	float4 top = COLOR_OFFSET(uv, float2(0, 1));
	float4 bottom = COLOR_OFFSET(uv, float2(0, -1));
		
	float lumaCenter = LUMA(center);
	float lumaLeft = LUMA(left);
	float lumaRight = LUMA(right);
	float lumaTop = LUMA(top);
	float lumaBottom = LUMA(bottom);
	
	float maxLuma = max(lumaCenter, max(max(lumaLeft, lumaRight), max(lumaTop, lumaBottom)));
	float minLuma = min(lumaCenter, min(min(lumaLeft, lumaRight), min(lumaTop, lumaBottom)));
	
	float lumaRange = maxLuma - minLuma;
	
	if(lumaRange < max(MIN_LUMA, maxLuma * MAX_THRESHOLD))
	{
		return center;
	}
	else
	{
		float4 leftTop = COLOR_OFFSET(uv, float2(-1, 1));
		float4 rightTop = COLOR_OFFSET(uv, float2(1, 1));
		float4 leftBottom = COLOR_OFFSET(uv, float2(-1, -1));
		float4 rightBottom = COLOR_OFFSET(uv, float2(1, -1));
		
		float lumaLeftTop = LUMA(leftTop);
		float lumaLeftBottom = LUMA(leftBottom);
		float lumaRightTop = LUMA(rightTop);
		float lumaRightBottom  = LUMA(rightBottom);
	   
		float lumaTopBottom = lumaTop + lumaBottom;
		float lumaLeftRight = lumaLeft + lumaRight;
		float lumaLeftCorners = lumaLeftTop + lumaLeftBottom;
		float lumaRightCorners = lumaRightTop + lumaRightBottom;
		float lumaTopCorners = lumaLeftTop + lumaRightTop;
		float lumaBottomCorners = lumaLeftBottom + lumaRightBottom;
	   
	   //apply a low pass filter and use the absolute value to see the strenght of the edge in that direction
		float horizontal = abs(-2.0f * lumaLeft + lumaLeftCorners) + 2.0f * abs(-2.0f * lumaCenter + lumaTopBottom) + abs(-2.0f * lumaRight + lumaRightCorners);
		float vertical = abs(-2.0f * lumaTop + lumaTopCorners) + 2.0f * abs(-2.0f * lumaCenter + lumaLeftRight) + abs(-2.0f * lumaBottom + lumaBottomCorners);
	   
		bool isHorizontal = horizontal >= vertical;
	   
		//if(isHorizontal)
		//	return float4(1, 0, 0, 1);
		//else
		//	return float4(0, 1, 0, 1);
		
		float luma1;
		float luma2;	
		float2 uvStep;
		float2 offset;
		float2 uvDir;
		
		if(isHorizontal)
		{
			luma1 = lumaBottom;
			luma2 = lumaTop;
			uvStep = float2(0, INV_HEIGHT);
			offset = float2(INV_WIDTH, 0);
			uvDir = float2(1, 0);
		}
		else
		{
			luma1 = lumaLeft;
			luma2 = lumaRight;
			uvStep = float2(INV_WIDTH, 0);
			offset = float2(0, INV_HEIGHT);
			uvDir = float2(0, 1);
		}

		float grad1 = abs(luma1 - lumaCenter);
		float grad2 = abs(luma2 - lumaCenter);
	   
		float lumaLocalAverage;
		float gradiantScaled;
		
		if(grad1 >= grad2)
		{
			gradiantScaled = 0.25f * grad1;
			uvStep = -uvStep;
			lumaLocalAverage = 0.5f * (luma1 + lumaCenter);
			//return float4(1, 0, 0, 1);
		}
		else
		{
			gradiantScaled = 0.25f * grad2;
			lumaLocalAverage = 0.5f * (luma2 + lumaCenter);
			//return float4(0, 1, 0, 1);
		}
		
		float2 currentUv = pIn.texCoord;
		currentUv += uvStep * 0.5f;
	   
		float2 uv1 = currentUv;
		float2 uv2 = currentUv;
	   
		bool reached1 = false;
		bool reached2 = false;
		
		float lumaEnd1 = 0.0f;
		float lumaEnd2 = 0.0f;
	   
		[unroll]
		for(uint i = 0; i < ITERATIONS; i++)
		{
			if(!reached1)
			{
				uv1 -= offset * gQuality[i];
				lumaEnd1 = LUMA(COLOR(uv1)) - lumaLocalAverage;
				reached1 = abs(lumaEnd1) > gradiantScaled;
			}
		
			if(!reached2)
			{
				uv2 += offset * gQuality[i];
				lumaEnd2 = LUMA(COLOR(uv2)) - lumaLocalAverage;
				reached2 = abs(lumaEnd2) > gradiantScaled;
			}

			if(reached1 && reached2)
			{
				break;
			}
		}
		
		float distance1 = dot(pIn.texCoord - uv1, uvDir);
		float distance2 = dot(uv2 - pIn.texCoord, uvDir);
	   
		float distanceFinal;
		float lumaEndFinal;
		if(distance1 < distance2)
		{
			distanceFinal = distance1;
			lumaEndFinal = lumaEnd1;
			//return float4(1, 0, 0, 1);
		}
		else
		{
			distanceFinal = distance2;
			lumaEndFinal = lumaEnd2;
			//return float4(0, 1, 0, 1);
		}
		
		float edgeLength = distance1 + distance2;
		
		float pixelOffset = -distanceFinal / edgeLength + 0.5;
	   
		bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;
		
		bool correctVariation = (lumaEndFinal < 0.0f) != isLumaCenterSmaller;
		
		float finalOffset = correctVariation ? pixelOffset : 0.0f;
	   
		float lumaAverage = (1.0f / 12.0f) * (2.0f * (lumaTopBottom + lumaLeftRight) + lumaTopCorners + lumaRightCorners);
		
		float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter)/lumaRange, 0.0f, 1.0f);
		float subPixelOffset2 = (-2.0f * subPixelOffset1 + 3.0f) * subPixelOffset1 * subPixelOffset1;
		
		float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;
		
		finalOffset = max(finalOffset, subPixelOffsetFinal);
		
		float2 finalUv = pIn.texCoord;
		finalUv += uvStep * finalOffset;
		
		return COLOR(finalUv);
	
	}
}






