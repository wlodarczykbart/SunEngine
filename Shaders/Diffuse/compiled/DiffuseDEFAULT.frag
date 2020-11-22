#pragma pack_matrix(row_major)

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


struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 normal : NORMAL;
};

float4 main(PS_In pIn) : SV_TARGET
{
	float4 normal = normalize(pIn.normal);	
	float diffuse = max(dot(normal, SunDirection), 0.0);
	float4 color = SunColor * diffuse;
	return color;
}


