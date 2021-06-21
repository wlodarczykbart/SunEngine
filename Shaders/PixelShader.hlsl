#ifndef GBUFFER
#include "LightBuffer.hlsl"
#include "EnvBuffer.hlsl"
#include "CameraBuffer.hlsl"
#include "ShadowMap.hlsl"
#endif

#include "Utility.hlsl"

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

void ShadePixel(float3 albedo, float ambient, float3 specular, float smoothness, float3 emissive, float3 normal, float3 position, float2 screenTexCoord, out PS_Out pOut)
{
#ifdef GBUFFER
	pOut.Albedo = float4(albedo, ambient);
	pOut.Specular = float4(specular, smoothness);
	pOut.Normal = float4(encodeNormal(normal), emissive.rg);
	pOut.Position = float4(position, emissive.b);
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
	float3 shadowFactor = ComputeShadowFactor(worldPos, position.z);
	
	float3 litColor = BRDF_CookTorrance(l, normal, v, SunColor.rgb, albedo, specular, smoothness, 0.001) * shadowFactor;
	
    float3 ambientColor = 0.01 * ambient * albedo;	
	
	float3 toPixel = (worldPos - InvViewMatrix[3]).xyz;
	
	pOut.color = float4(litColor + ambientColor + emissive, 1.0);	
	pOut.color.rgb = ComputeFogContribution(pOut.color.rgb, toPixel, InvViewMatrix[3].y);
	//pOut.color.rgb = lerp(pOut.color.rgb, FogColor.rgb, 0.5);
	//pOut.color.rgb = normal;
#endif
}
