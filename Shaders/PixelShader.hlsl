#include "CameraBuffer.hlsl"
#include "SunlightBuffer.hlsl"
#include "ShadowMap.hlsl"
#include "FogBuffer.hlsl"

struct PixelOutput
{
	float4 albedo;
	float4 specular;
	float4 worldPosition;
	float4 normal;
};

float3x3 BuildTBN(float4 normal, float4 tangent)
{
	float3x3 mtx;
	float3 t = tangent.xyz;
	float t2 = dot(t, t);
	t = t2 > 0.0 ? t / sqrt(t2) : float3(0, 0, 0);
	mtx[0] = t;
	mtx[2] = normalize(normal.xyz);
	mtx[1] = cross(mtx[2], mtx[0]);
	return mtx;
}

float4 TransformNormal(float4 bumpNormal, float4 normal, float4 tangent)
{
	return float4(normalize(mul(bumpNormal.xyz * 2.0 - 1.0, BuildTBN(normal, tangent))), 0.0);
}

##INJECT_PS_IN##

PixelOutput ps(PS_In pIn);

float4 main(PS_In pIn) : SV_TARGET
{
	PixelOutput data = ps(pIn);
	
#ifdef APPLY_ALPHA_TEST
	if(data.albedo.a < 0.0)
		discard;
#endif
	
	float4 outColor = data.albedo;
	
	float3 viewVector = (InvViewMatrix[3] - data.worldPosition).xyz;	
	float distanceToCamera = length(viewVector);
	viewVector /= distanceToCamera;
	
#ifdef APPLY_LIGHTING

	float3 normal = normalize(data.normal.xyz);
	outColor = float4(ComputeSunlightContribution(viewVector, normal, data.albedo.rgb, data.specular), data.albedo.a);
#endif
	
#ifdef APPLY_SHADOWS	
	float4 shadowCoord = mul(data.worldPosition, ShadowMatrix); 
	shadowCoord /= shadowCoord.w;
	if(!(
		shadowCoord.x < -1.0 || shadowCoord.x > 1.0 || 
		shadowCoord.y < -1.0 || shadowCoord.y > 1.0 ||
		shadowCoord.z < 0.0 || shadowCoord.z > 1.0))
	{
		shadowCoord = shadowCoord * 0.5 + 0.5;
		shadowCoord.y = 1.0 - shadowCoord.y;		
		float sDepth = SampleShadow(shadowCoord.xy);	
		
		//shadowCoord.z -= 0.3;
		if(sDepth < shadowCoord.z)
		{
			outColor *= 0.7;
		}
	}
#endif	
	
	
#ifdef APPLY_FOG
	viewVector = (InvViewMatrix[3] - data.worldPosition).xyz;	
	viewVector.y *= 0.1;
	distanceToCamera = length(viewVector);
	outColor.rgb = ComputeFogContribution(outColor.rgb, distanceToCamera);
#endif		

#ifdef RENDER_NORMALS
	outColor = float4(normalize(data.normal.xyz), 1.0);
#endif

#ifdef RENDER_VIEW_VECTOR
	outColor = float4(normalize((InvViewMatrix[3] - data.worldPosition).xyz), 1.0);
#endif

	return outColor;	
}
