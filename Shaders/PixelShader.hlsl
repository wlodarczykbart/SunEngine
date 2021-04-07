#ifndef DEFERRED
#include "LightBuffer.hlsl"
#include "CameraBuffer.hlsl"
#endif

struct PS_Out
{
#ifdef DEFERRED
	float4 Albedo : SV_TARGET0;
	float4 Specular : SV_TARGET1;
	float4 Normal : SV_TARGET2;
	float4 Position : SV_TARGET3;
#else
	float4 color : SV_TARGET;
#endif	
};

void ShadePixel(float3 albedo, float ambient, float3 specular, float smoothness, float3 normal, float3 position, out PS_Out pOut)
{
#ifdef DEFERRED
	pOut.Albedo = float4(albedo, ambient);
	pOut.Specular = float4(specular, smoothness);
	pOut.Normal = float4(normal, 0.0);
	pOut.Position = float4(position, 0.0);
#else
	float3 l = normalize(SunDirection.xyz);	
	float3 v = normalize(InvViewMatrix[3].xyz - position);
	float3 litColor = BRDF_CookTorrance(l, normal, v, SunColor.rgb, albedo, specular, smoothness, 0.001);
	
    float3 ambientColor = 0.03 * albedo * ambient;	
	
	pOut.color = float4(litColor + ambientColor, 1.0);
#endif
}
