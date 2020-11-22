#pragma pack_matrix(row_major)
#define APPLY_SHADOWS 1




[[vk::binding(0, 0)]]
cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};

[[vk::binding(1, 0)]]
cbuffer SunlightBuffer
 : register(b3){
	float4 SunDirection;
	float4 SunColor;
};

float3 ComputeSunlightContribution(float3 viewVector, float3 normal, float3 albedo, float4 specular)
{
	float3 sunDir = normalize(SunDirection.xyz);
	
	float diffuseComponent = max(dot(normal, sunDir), 0.1);
	float3 diffuseColor = albedo * diffuseComponent;
	diffuseColor *= SunColor.rgb * SunColor.a;
	
	float3 halfVec = normalize(viewVector + sunDir);
	float specularComponent = max(dot(normal, halfVec), 0.0);
	specularComponent = pow(specularComponent, specular.a);
	float3 specularColor = specular.rgb * specularComponent;
	specularColor *= SunColor.rgb * SunColor.a;
	
	return diffuseColor + specularColor;
}

[[vk::binding(2, 1)]]
Texture2D ShadowTexture : register(t2);
[[vk::binding(2, 2)]]
SamplerState ShadowSampler : register(s2);

[[vk::binding(6, 0)]]
cbuffer ShadowBuffer
 : register(b10){
	float4x4 ShadowMatrix;
};

float SampleShadow(float2 texCoord)
{
	return ShadowTexture.Sample(ShadowSampler, texCoord).r;
}

#define FOG_DENSITY FogControls.x
#define FOG_ENABLED (FogControls.y > 0.0001)

[[vk::binding(4, 0)]]
cbuffer FogBuffer
 : register(b7){
	float4 FogColor;
	float4 FogControls;
};

float3 ComputeFogContribution(float3 inputColor, float distanceToCamera)
{
	if(FOG_ENABLED)
	{
		float fogFactor =  1.0 - clamp(exp(-distanceToCamera * FOG_DENSITY), 0.0, 1.0);
		return lerp(inputColor, FogColor.rgb, fogFactor);
	}
	else
	{
		return inputColor;
	}
}


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

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 normal : NORMAL;
	float4 position : POSITION;
};

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



[[vk::binding(0, 6)]]
cbuffer MaterialBuffer
 : register(b2){
	float4 Color;
};

PixelOutput ps(PS_In pIn)
{
	PixelOutput pOutput;
	pOutput.albedo = Color;
	pOutput.normal = normalize(pIn.normal);
	pOutput.specular = float4(0, 0, 0, 1);
	pOutput.worldPosition = pIn.position;
	return pOutput;
}





