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
	float ComputeShadowFactor(float4 fragPos, float nDotL)
	{
		float4 shadowCoord = mul(fragPos, ShadowMatrices[0]);
		shadowCoord /= shadowCoord.w;
		shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
		
		if(
			shadowCoord.x < 0.0 || 
			shadowCoord.x > 1.0 || 
			shadowCoord.y < 0.0 || 
			shadowCoord.y > 1.0 || 
			shadowCoord.z < 0.0 || 
			shadowCoord.z > 1.0)
		{
			return 1.0;
		}
		
		shadowCoord.y = 1.0 - shadowCoord.y;
		float depth = ShadowTexture.Sample(ShadowSampler, shadowCoord.xy).r;
		
		float bias = max(0.05 * (1.0 - nDotL), 0.005);
		if(depth < shadowCoord.z - bias) 
			return 0.0;
		else
			return 1.0;
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

	float3 litColor = BRDF_CookTorrance(l, normal, v, SunColor.rgb, albedo, specular, smoothness, 0.001);
	
    float3 ambientColor = 0.03 * albedo * ambient;	
	
	pOut.color = float4(litColor + ambientColor, 1.0);
	
	float4 worldPos = mul(float4(position, 1.0), InvViewMatrix);
	pOut.color *= ComputeShadowFactor(worldPos, max(dot(l, normalize(normal)), 0.0));
	
#endif
}
