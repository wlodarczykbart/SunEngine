#ifndef GBUFFER
#include "LightBuffer.hlsl"
#include "CameraBuffer.hlsl"
#include "ShadowMap.hlsl"
#endif

struct PS_Out
{
#ifdef GBUFFER
	float4 Albedo : SV_TARGET0;
	float4 Specular : SV_TARGET1;
	float4 Normal : SV_TARGET2;
	float4 Position : SV_TARGET3;
#else
	float4 color : SV_TARGET;
#endif	
};

#ifndef GBUFFER
	float ComputeShadowFactor(float4 fragPos)
	{
		float4 shadowCoord = mul(fragPos, ShadowMatrices[0]);
		shadowCoord /= shadowCoord.w;
		
		if(
			shadowCoord.z < 0.0 || 
			shadowCoord.z > 1.0)
		{
			return 1.0;
		}	
		
		shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
		shadowCoord.y = 1.0 - shadowCoord.y;
		
		float2 texSize;
		ShadowTexture.GetDimensions(texSize.x, texSize.y);
		texSize = 1.0 / texSize;
		texSize *= 1.5;
		
		int range = 1;
		float inShadow = 0.0;	
		float count = 0.0;
		
		[unroll]
		for(int i = -range; i <= range; i++)
		{
			for(int j = -range; j <= range; j++)
			{
				float depth = ShadowTexture.Sample(ShadowSampler, shadowCoord.xy + float2(i, j) * texSize).r;		
				if(shadowCoord.z > depth) 
					inShadow += 1.0;
				count += 1.0;
			}
		}
		
		return 1.0 - (inShadow / count);
	}
#endif

void ShadePixel(float3 albedo, float ambient, float3 specular, float smoothness, float3 normal, float3 position, out PS_Out pOut)
{
#ifdef GBUFFER
	pOut.Albedo = float4(albedo, ambient);
	pOut.Specular = float4(specular, smoothness);
	pOut.Normal = float4(normal, 0.0);
	pOut.Position = float4(position, 0.0);
#else

#if 1
	float3 l = normalize(SunViewDirection.xyz);	
	float3 v = -normalize(position.xyz);
#else
	float3 l = normalize(SunDirection.xyz);	
	float3 v = normalize(InvViewMatrix[3].xyz - position);
#endif	

	float shadowFactor = ComputeShadowFactor(mul(float4(position, 1.0), InvViewMatrix));
	float3 litColor = BRDF_CookTorrance(l, normal, v, SunColor.rgb, albedo, specular, smoothness, 0.001) * shadowFactor;
	
    float3 ambientColor = 0.03 * albedo * ambient;	
	
	pOut.color = float4(litColor + ambientColor, 1.0);	
#endif
}
