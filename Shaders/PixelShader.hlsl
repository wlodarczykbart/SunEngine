#ifndef GBUFFER
#include "EnvBuffer.hlsl"
#include "LightBuffer.hlsl"
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

void ShadePixel(float3 albedo, float ambient, float3 specular, float smoothness, float3 emissive, float3 normal, float3 position, out PS_Out pOut)
{
#ifdef GBUFFER
	pOut.Albedo = float4(albedo, ambient);
	pOut.Specular = float4(specular, smoothness);
	pOut.Normal = float4(encodeNormal(normal), emissive.rg);
	pOut.Position = float4(position, emissive.b);
#else

	float distToEye = 0.0;
#if 1
	float3 l = SUN_VIEW_DIRECTION.xyz;	
	distToEye = length(position);
	float3 v = -(position / distToEye);
#else
	float3 l = normalize(SunDirection.xyz);	
	float3 v = InvViewMatrix[3].xyz - position;
	distToEye = length(v);
	v /= distToEye;
#endif	

#ifndef SIMPLE_SHADING
	float4 worldPos = mul(float4(position, 1.0), InvViewMatrix);
	float3 shadowFactor = ComputeShadowFactor(worldPos, position.z);
	
	//TODO: is there some way to do this without moving to world space?
	float3 r = normalize(mul(float4(reflect(-v, normal), 0.0), InvViewMatrix).xyz);	
	
	float3 litColor = BRDF_CookTorrance(l, normal, v, r, SunColor.rgb, albedo, specular, smoothness, 0.001) * shadowFactor;
	
	float nDotV = max(dot(normal, v), EPSILON);
	float3 F = F_SchlickR(nDotV, specular, 1.0-smoothness);
	float rdist = length(worldPos.xyz - EnvProbeCenters[0].xyz);
	float ra = 1.0 - saturate(rdist / 15.0);
	float3 reflection = ra < 0.999 ? SampleEnvironment(r, 0) * ra : SampleEnvironment(r);	
	
	reflection *= F;
	//reflection *= 0;
	
    float3 ambientColor = 0.01 * ambient * albedo + reflection;	
	
	float3 toPixel = (worldPos - InvViewMatrix[3]).xyz;
	
	pOut.color = float4(litColor + ambientColor + emissive, 1.0);

	//pOut.color.rgb = lerp(pOut.color.rgb, float3(ra,ra,ra), dot(r,r));
	
	pOut.color.rgb = ComputeFogContribution(pOut.color.rgb, toPixel, InvViewMatrix[3].y);
#else
	float3 h = normalize(l + v);
	float3 d = max(dot(normal, l), 0.025) * albedo * SunColor.rgb;	
	float3 s = pow(max(dot(normal, h), 0.0), lerp(1, 1024, smoothness)) * specular * SunColor.rgb;
    float3 a = 0.01 * ambient * albedo;

	pOut.color = float4(lerp(d + s + a + emissive, d, 1.0-dot(h,h)), 1.0);
#endif	

#endif
}
