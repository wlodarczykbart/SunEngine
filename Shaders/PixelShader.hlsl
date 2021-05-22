#ifndef GBUFFER
#include "LightBuffer.hlsl"
#include "EnvBuffer.hlsl"
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
			shadowCoord.w < 0.0 ||
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

void ShadePixel(float3 albedo, float ambient, float3 specular, float smoothness, float3 normal, float3 position, float2 screenTexCoord, out PS_Out pOut)
{
#ifdef GBUFFER
	pOut.Albedo = float4(albedo, ambient);
	pOut.Specular = float4(specular, smoothness);
	pOut.Normal = float4(normal, 0.0);
	pOut.Position = float4(position, 0.0);
#else

	float distToEye = 0.0;
#if 1
	float3 l = normalize(SunViewDirection.xyz);	
	distToEye = length(position);
	float3 v = -(position / distToEye);
#else
	float3 l = normalize(SunDirection.xyz);	
	float3 v = InvViewMatrix[3].xyz - position;
	distToEye = length(v);
	v /= distToEye;
#endif	

	float4 worldPos = mul(float4(position, 1.0), InvViewMatrix);
	float shadowFactor = ComputeShadowFactor(worldPos);
	float3 litColor = BRDF_CookTorrance(l, normal, v, SunColor.rgb, albedo, specular, smoothness, 0.001) * shadowFactor;
	
    float3 ambientColor = 0.03 * albedo * ambient;	
	
	//float3 EnvColor = float3(0.5,0.5,0.5);
	//float ft = 1.0-exp(-distToEye * 0.005);
	//float heightFactor = exp(-worldPos.y * 0.1);
	//EnvColor = lerp(EnvColor, float3(0.1, 0.1, 0.1), 1.0-heightFactor);
	
	pOut.color = float4(litColor + ambientColor, 1.0);	
	pOut.color.rgb = ComputeFogContribution(pOut.color.rgb, distToEye, screenTexCoord);
#endif
}
