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

[[vk::binding(0, 0)]]
cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};


struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 direction : DIRECTION;
};

#if 1

#define M_PI 3.1415926535897932384626433832795

#define FLOAT3_ZERO float3(0,0,0)

bool solveQuadratic(float a, float b, float c, out float x1, out float x2)
{
	if (b == 0) {
		// Handle special case where the the two vector ray.dir and V are perpendicular
		// with V = ray.orig - sphere.centre
		if (a == 0) return false;
		x1 = 0; x2 = sqrt(-c / a);
		return true;
	}
	float discr = b * b - 4 * a * c;

	if (discr < 0) return false;

	float q = (b < 0.f) ? -0.5f * (b - sqrt(discr)) : -0.5f * (b + sqrt(discr));
	x1 = q / a;
	x2 = c / q;

	return true;
}

bool raySphereIntersect(float3 orig, float3 dir, float radius, out float t0, out float t1)
{
	// They ray dir is normalized so A = 1 
	float A = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
	float B = 2 * (dir.x * orig.x + dir.y * orig.y + dir.z * orig.z);
	float C = orig.x * orig.x + orig.y * orig.y + orig.z * orig.z - radius * radius;

	if (!solveQuadratic(A, B, C, t0, t1)) return false;

	if (t0 > t1)
	{
		float tmp = t0;
		t0=t1;
		t1=tmp;
	}

	return true;
}

static float earthRadius = 6360e3;
static float atmosphereRadius = 6420e3;
static float Hr = 7994;
static float Hm = 1200;

static float3 betaR = float3(3.8e-6f, 13.5e-6f, 33.1e-6f);
static float3 betaM = float3(21e-6f, 21e-6f, 21e-6f);

static float kInfinity = (1e+300*1e+300);

//derived from https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
float3 computeIncidentLight(float3 orig, float3 dir, float tmin, float tmax)
{
	float3 sunDirection = normalize(SunDirection.xyz);
	float t0, t1;
	if (!raySphereIntersect(orig, dir, atmosphereRadius, t0, t1) || t1 < 0) return 0;
	if (t0 > tmin && t0 > 0) tmin = t0;
	if (t1 < tmax) tmax = t1;
	uint numSamples = 16;
	uint numSamplesLight = 8;
	float segmentLength = (tmax - tmin) / numSamples;
	float tCurrent = tmin;
	float3 sumR = float3(0, 0, 0);
	float3 sumM = float3(0, 0, 0); // mie and rayleigh contribution
	float opticalDepthR = 0, opticalDepthM = 0;
	float mu = dot(dir, sunDirection); // mu in the paper which is the cosine of the angle between the sun direction and the ray direction
	float phaseR = 3.f / (16.f * M_PI) * (1 + mu * mu);
	float g = 0.76f;
	float phaseM = 3.f / (8.f * M_PI) * ((1.f - g * g) * (1.f + mu * mu)) / ((2.f + g * g) * pow(1.f + g * g - 2.f * g * mu, 1.5f));
	for (uint i = 0; i < numSamples; ++i) {
		float3 samplePosition = orig + (tCurrent + segmentLength * 0.5f) * dir;
		float height = length(samplePosition) - earthRadius;
		// compute optical depth for light
		float hr = exp(-height / Hr) * segmentLength;
		float hm = exp(-height / Hm) * segmentLength;
		opticalDepthR += hr;
		opticalDepthM += hm;
		// light optical depth
		float t0Light, t1Light;
		raySphereIntersect(samplePosition, sunDirection, atmosphereRadius, t0Light, t1Light);
		float segmentLengthLight = t1Light / numSamplesLight, tCurrentLight = 0;
		float opticalDepthLightR = 0, opticalDepthLightM = 0;
		uint j;
		for (j = 0; j < numSamplesLight; ++j) {
			float3 samplePositionLight = samplePosition + (tCurrentLight + segmentLengthLight * 0.5f) * sunDirection;
			float heightLight = length(samplePositionLight) - earthRadius;
			if (heightLight < 0) break;
			opticalDepthLightR += exp(-heightLight / Hr) * segmentLengthLight;
			opticalDepthLightM += exp(-heightLight / Hm) * segmentLengthLight;
			tCurrentLight += segmentLengthLight;
		}
		if (j == numSamplesLight) {
			float3 tau = betaR * (opticalDepthR + opticalDepthLightR) + betaM * 1.1f * (opticalDepthM + opticalDepthLightM);
			float3 attenuation = float3(exp(-tau.x), exp(-tau.y), exp(-tau.z));
			sumR += attenuation * hr;
			sumM += attenuation * hm;
		}
		tCurrent += segmentLength;
	}

	// [comment]
	// We use a magic number here for the intensity of the sun (20). We will make it more
	// scientific in a future revision of this lesson/code
	// [/comment]
	return (sumR * betaR * phaseR + sumM * betaM * phaseM) * 20;
}

float4 main(PS_In pIn) : SV_TARGET
{
	float3 orig = float3(0, earthRadius + 1000, 30000); // camera position	
	float3 dir = normalize(pIn.direction.xyz);	
	float t0, t1, tMax = kInfinity;
	if (raySphereIntersect(orig, dir, earthRadius, t0, t1) && t1 > 0)
		tMax = max(0.f, t0);
	// [comment]
	// The *viewing or camera ray* is bounded to the range [0:tMax]
	// [/comment]
	float3 color = computeIncidentLight(orig, dir, 0, tMax);
	float3 sunColor = float3(1,1,1) * step(cos(3.1415926 / 180.0), dot(dir, SunDirection.xyz)) * 20;
	color += sunColor;
	
	color = 1.0 - exp(-1.0 * color);	
	return float4(color, 1.0);
}

#else

#define PI 3.141592
#define iSteps 16
#define jSteps 8

float2 rsi(float3 r0, float3 rd, float sr) {
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(rd, rd);
    float b = 2.0 * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b*b) - 4.0*a*c;
    if (d < 0.0) return float2(1e5,-1e5);
    return float2(
        (-b - sqrt(d))/(2.0*a),
        (-b + sqrt(d))/(2.0*a)
    );
}

float3 atmosphere(float3 r, float3 r0, float3 pSun, float iSun, float rPlanet, float rAtmos, float3 kRlh, float kMie, float shRlh, float shMie, float g) {
    // Normalize the sun and view directions.
    pSun = normalize(pSun);
    r = normalize(r);

    // Calculate the step size of the primary ray.
    float2 p = rsi(r0, r, rAtmos);
    if (p.x > p.y) return float3(0,0,0);
    p.y = min(p.y, rsi(r0, r, rPlanet).x);
    float iStepSize = (p.y - p.x) / float(iSteps);

    // Initialize the primary ray time.
    float iTime = 0.0;

    // Initialize accumulators for Rayleigh and Mie scattering.
    float3 totalRlh = float3(0,0,0);
    float3 totalMie = float3(0,0,0);

    // Initialize optical depth accumulators for the primary ray.
    float iOdRlh = 0.0;
    float iOdMie = 0.0;

    // Calculate the Rayleigh and Mie phases.
    float mu = dot(r, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
    float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

    // Sample the primary ray.
    for (int i = 0; i < iSteps; i++) {

        // Calculate the primary ray sample position.
        float3 iPos = r0 + r * (iTime + iStepSize * 0.5);

        // Calculate the height of the sample.
        float iHeight = length(iPos) - rPlanet;

        // Calculate the optical depth of the Rayleigh and Mie scattering for this step.
        float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
        float odStepMie = exp(-iHeight / shMie) * iStepSize;

        // Accumulate optical depth.
        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        // Calculate the step size of the secondary ray.
        float jStepSize = rsi(iPos, pSun, rAtmos).y / float(jSteps);

        // Initialize the secondary ray time.
        float jTime = 0.0;

        // Initialize optical depth accumulators for the secondary ray.
        float jOdRlh = 0.0;
        float jOdMie = 0.0;

        // Sample the secondary ray.
        for (int j = 0; j < jSteps; j++) {

            // Calculate the secondary ray sample position.
            float3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);

            // Calculate the height of the sample.
            float jHeight = length(jPos) - rPlanet;

            // Accumulate the optical depth.
            jOdRlh += exp(-jHeight / shRlh) * jStepSize;
            jOdMie += exp(-jHeight / shMie) * jStepSize;

            // Increment the secondary ray time.
            jTime += jStepSize;
        }

        // Calculate attenuation.
        float3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

        // Accumulate scattering.
        totalRlh += odStepRlh * attn;
        totalMie += odStepMie * attn;

        // Increment the primary ray time.
        iTime += iStepSize;

    }

    // Calculate and return the final color.
    return iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
}

float4 main(PS_In pIn) : SV_TARGET
{
	float3 vPosition = pIn.rayO.xyz;
	    
	float3 color = atmosphere(
        vPosition,           			// normalized ray direction
        float3(0,6372e3,0),               // ray origin
        SunDirection.xyz,                        // position of the sun
        22.0,                           // intensity of the sun
        6371e3,                         // radius of the planet in meters
        6471e3,                         // radius of the atmosphere in meters
        float3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
        21e-6,                          // Mie scattering coefficient
        8e3,                            // Rayleigh scale height
        1.2e3,                          // Mie scale height
        0.758                           // Mie preferred scattering direction
    );

    // Apply exposure.
    color = 1.0 - exp(-1.0 * color);
	return float4(color, 1.0);
}
#endif


