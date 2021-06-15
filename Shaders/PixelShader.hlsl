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

float hash( float2 inCoord ) 
{
	return frac( 1.0e4 * sin( 17.0*inCoord.x + 0.1*inCoord.y ) *
		( 0.1 + abs( sin( 13.0*inCoord.y + inCoord.x )))
	);
}

float hash3D( float3 inCoord ) 
{
	return hash( float2( hash( inCoord.xy ), inCoord.z ) );
}

float CalcAlphaThreshold(float3 objCoord)
{
	float g_HashScale = 1.0;
	
	// Find the discretized derivatives of our coordinates
	float maxDeriv = max( length(ddy(objCoord)),
	length(ddy(objCoord)) );
	float pixScale = 1.0/(g_HashScale*maxDeriv);
	// Find two nearest log-discretized noise scales
	float2 pixScales = float2( exp2(floor(log2(pixScale))),
	exp2(ceil(log2(pixScale))) );
	// Compute alpha thresholds at our two noise scales
	float2 alpha=float2(hash3D(floor(pixScales.x*objCoord)),
	hash3D(floor(pixScales.y*objCoord)));
	// Factor to interpolate lerp with
	float lerpFactor = frac( log2(pixScale) );
	// Interpolate alpha threshold from noise at two scales
	float x = (1-lerpFactor)*alpha.x + lerpFactor*alpha.y;
	// Pass into CDF to compute uniformly distrib threshold
	float a = min( lerpFactor, 1-lerpFactor );
	float3 cases = float3( x*x/(2*a*(1-a)),
	(x-0.5*a)/(1-a),
	1.0-((1-x)*(1-x)/(2*a*(1-a))) );
	// Find our final, uniformly distributed alpha threshold
	float t = (x < (1-a)) ?
	((x < a) ? cases.x : cases.y) :
	cases.z;
	// Avoids t == 0. Could also do t =1-t
	return clamp( t , 1.0e-6, 1.0 );
}

float CalcMipLevel(float2 texture_coord)
{
	float2 dx = ddx(texture_coord);
	float2 dy = ddy(texture_coord);
	float delta_max_sqr = max(dot(dx, dx), dot(dy, dy));
	
	return max(0.0, 0.5 * log2(delta_max_sqr));
}

//from https://aras-p.info/texts/CompactNormalStorage.html
float2 encode (float3 n)
{
    float p = sqrt(n.z*8+8);
    return float2(n.xy/p + 0.5);
}

float3 decode (float2 enc)
{
    float2 fenc = enc*4-2;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
    float3 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}

void ShadePixel(float3 albedo, float ambient, float3 specular, float smoothness, float3 normal, float3 position, float2 screenTexCoord, out PS_Out pOut)
{
#ifdef GBUFFER
	pOut.Albedo = float4(albedo, ambient);
	pOut.Specular = float4(specular, smoothness);
	pOut.Normal = float4(encode(normal), 0.0, 0.0);
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
	float3 shadowFactor = ComputeShadowFactor(position);
	
	float3 litColor = BRDF_CookTorrance(l, normal, v, SunColor.rgb, albedo, specular, smoothness, 0.001) * shadowFactor;
	
    float3 ambientColor = 0.03 * ambient * albedo;	
	
	//float3 EnvColor = float3(0.5,0.5,0.5);
	//float ft = 1.0-exp(-distToEye * 0.005);
	//float heightFactor = exp(-worldPos.y * 0.1);
	//EnvColor = lerp(EnvColor, float3(0.1, 0.1, 0.1), 1.0-heightFactor);
	
	pOut.color = float4(litColor + ambientColor, 1.0);	
	pOut.color.rgb = ComputeFogContribution(pOut.color.rgb, distToEye, screenTexCoord);
	//pOut.color.rgb = normal;
#endif
}
