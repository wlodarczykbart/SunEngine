cbuffer SunlightBuffer
{
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