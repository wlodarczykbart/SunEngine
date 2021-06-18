//http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html

#define PI 3.1415926535897932384626433832795
#define EPSILON 0.00001
#define GAMMA_V float3(2.2, 2.2, 2.2)

float D_GGX(float nDotH, float a)
{
	float a2 = a*a;
		
	float denom = (nDotH * nDotH) * (a2 - 1) + 1;
	float ndf = a2 / (PI * denom * denom);
	return ndf;
}

float G_GGX(float cosAngle, float a)
{
	float a2 = a*a;
	
	float numer = 2.0 * cosAngle;
	float denom = cosAngle + sqrt(a2 + ((1.0 - a2) * (cosAngle * cosAngle)));
	
	return numer / denom;
}

float G_SchlickGGX(float cosAngle, float a)
{
    float r = (a + 1.0);
    float k = (r*r) / 8.0;

    float num   = cosAngle;
    float denom = cosAngle * (1.0 - k) + k;
	
    return num / denom;
}

float G_Smith(float nDotV, float nDotL, float a)
{
	return G_GGX(nDotV, a) * G_GGX(nDotL, a);
}

float G_SchlicksmithGGX(float nDotV, float nDotL, float a)
{
	float r = (a + 1.0);
	float k = (r*r) / 8.0;
	float GL = nDotL / (nDotL * (1.0 - k) + k);
	float GV = nDotV / (nDotV * (1.0 - k) + k);
	return GL * GV;
}


float3 F_Schlick(float vDotH, float3 f0)
{
	return f0 + (float3(1,1,1) - f0) * pow(1.0 - vDotH, 5.0);
}

float3 BRDF_CookTorrance(float3 l, float3 n, float3 v, float3 lightColor, float3 albedo, float3 f0, float smoothness, float attenuation)
{
	l  = normalize(l);
	n = normalize(n);
	v = normalize(v);
	float3 h = normalize(l + v);
	
	smoothness = clamp(smoothness, EPSILON, 1.0 - EPSILON);
	float a = 1.0 - smoothness;
	a = a * a * a;	
	
	float vDotH = max(dot(v, h), EPSILON);
	float nDotV = max(dot(n, v), EPSILON);
	float nDotL = max(dot(n, l), EPSILON);
	float nDotH = max(dot(n, h), EPSILON);
	
	float3 F = F_Schlick(vDotH, f0);
	float D = D_GGX(nDotH, a);
	float G = G_SchlicksmithGGX(nDotV, nDotL, a);
	
	float3 specular = (D * G * F) / (4.0 * nDotL * nDotV);
	float3 diffuse = albedo / PI;
	
	float3 kD = float3(1, 1, 1) - F; //metallic is handled in the albedo calculation 
	return (kD * albedo / PI + specular) * nDotL * lightColor;
	
	
	//float3 kS = F;
	//kS = float3(1,1,1) * dot(kS, float3(0.2126, 0.7152, 0.0722));
	//float3 kD = float3(1,1,1) - kS;
	
	//return specular * nDotL * lightColor;
	//return lightColor * nDotL * (diffuse * kD + specular);
	//return lightColor * nDotL * (diffuse * (1.0f - specular) + specular);
}
