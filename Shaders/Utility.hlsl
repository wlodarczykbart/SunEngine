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
float2 encodeNormal(float3 n)
{
    float p = sqrt(n.z*8+8);
    return float2(n.xy/p + 0.5);
}

float3 decodeNormal(float2 enc)
{
    float2 fenc = enc*4-2;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
    float3 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}

//from http://diaryofagraphicsprogrammer.blogspot.com/2009/10/bitmasks-packing-data-into-fp-render.html
float encodeColor(float3 channel)
{
	// layout of a 32-bit fp register
	// SEEEEEEEEMMMMMMMMMMMMMMMMMMMMMMM
	// 1 sign bit; 8 bits for the exponent and 23 bits for the mantissa
	uint uValue;

	// pack x
	uValue = ((uint)(channel.x * 65535.0 + 0.5)); // goes from bit 0 to 15

	// pack y in EMMMMMMM
	uValue |= ((uint)(channel.y * 255.0 + 0.5)) << 16;

	// pack z in SEEEEEEE
	// the last E will never be 1b because the upper value is 254
	// max value is 11111110 == 254
	// this prevents the bits of the exponents to become all 1
	// range is 1.. 254
	// to prevent an exponent that is 0 we add 1.0
	uValue |= ((uint)(channel.z * 253.0 + 1.5)) << 24;

	return asfloat(uValue);
};

float3 decodeColor(float fFloatFromFP32)
{
	float a, b, c, d;
	uint uValue;

	uint uInputFloat = asuint(fFloatFromFP32);

	// unpack a
	// mask out all the stuff above 16-bit with 0xFFFF
	a = ((uInputFloat) & 0xFFFF) / 65535.0;

	b = ((uInputFloat >> 16) & 0xFF) / 255.0;

	// extract the 1..254 value range and subtract 1
	// ending up with 0..253
	c = (((uInputFloat >> 24) & 0xFF) - 1.0) / 253.0;

	return float3(a, b, c);
}