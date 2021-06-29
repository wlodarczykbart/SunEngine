#pragma pack_matrix(row_major)

struct PS_In
{
    float4 clipPos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

Texture2D ColorTexture : register(t0);
Texture2D PositionTexture : register(t1);
Texture2D NormalTexture : register(t2);
Texture2D DepthTexture : register(t3);
Texture2D BackfaceDepthTexture : register(t4);
SamplerState Sampler : register(s0);

cbuffer CameraBuffer : register(b0)
{
	float4x4 ViewProjMatrix;
    float4x4 ViewMatrix;
    float4x4 ProjMatrix;
    float4x4 InvProjMatrix;
    float4x4 ProjPixelMatrix;
}

float GetEyeDepth(Texture2D tex, int2 texCoord)
{
    //Could just sample PositionTexture, but doing this to validate it also works...

    /*
    proj.z = eye.x * 0 + eye.y * 0 + eye.z * proj22 + proj32
    proj.z = eye.z * proj22 + proj32

    proj.w = eye.x * 0 + eye.y * 0 + eye.z * proj23(should be -1) + 0
    proj.w = -eye.z
    depth = (eye.z * proj22 + proj32) / -eye.z
    depth * -eye.z = eye.z * proj22 + proj32
    depth * -eye.z - eye.z * proj22 = proj32
    -eye.z(depth + proj22) = proj32
    -eye.z = proj32 / depth + proj22 
    */

    float depth = tex.Load(int3(texCoord, 0)).r;
    return ProjMatrix[2][3] * ProjMatrix[3][2] / (depth + ProjMatrix[2][2]);
}

bool traceScreenSpaceRay1(
    float3 csOrig,
    float3 csDir,
    float4x4 proj,
    Texture2D csZBuffer,
    float2 csZBufferSize,
    float zThickness,
    float nearPlaneZ,
    float stride,
    float jitter,
    const float maxSteps,
    float maxDistance,
    out float2 hitPixel,
    out float3 hitPoint);


bool traceScreenSpaceRay2(
    float3          csOrigin,
    float3         csDirection,
    float4x4        projectToPixelMatrix,
    Texture2D       csZBuffer,
    float2          csZBufferSize,
    float           csZThickness,
    float           nearPlaneZ,
    float			stride,
    float           jitterFraction,
    float           maxSteps,
    in float        maxRayTraceDistance,
    out float2      hitPixel,
    out float3		csHitPoint);

bool ssr(in float2 texCoord, in float2 texSize, out float2 outCoord, out float outVisibility);

bool traceScreenSpaceRay(
    // Camera-space ray origin, which must be within the view volume
    float3 csOrig,
    // Unit length camera-space ray direction
    float3 csDir,
    // Number between 0 and 1 for how far to bump the ray in stride units
    // to conceal banding artifacts. Not needed if stride == 1.
    float jitter,
    float MaxDistance,
    float NearPlaneZ,
    float2 RTSize,
    float StrideZCutoff,
    float Stride,
    float MaxSteps,
    float ZThickness,
    // Pixel coordinates of the first intersection with the scene
    out float2 hitPixel,
    // Camera space location of the ray hit
    out float3 hitPoint);

float4 main(PS_In pIn) : SV_TARGET
{
    float2 texSize;
    ColorTexture.GetDimensions(texSize.x, texSize.y);
    
    float2 texCoord = pIn.texCoord;
    float3 eyePos = PositionTexture.Sample(Sampler, texCoord).xyz;
    
    //float depth = DepthTexture.Sample(Sampler, texCoord).r;
    //float4 ndc = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, depth, 1.0f);
    //ndc = mul(ndc, InvProjMatrix);
    //ndc /= ndc.w;
    //eyePos = ndc.xyz;

    float3 eyeDir = normalize(NormalTexture.Sample(Sampler, texCoord).xyz);
    eyeDir = mul(float4(0, 1, 0, 0), ViewMatrix).xyz;
    float3 rayDir = normalize(reflect(eyePos, eyeDir));

   // return float4(eyePos, 1.0);

    float2 hitPixel;
    float3 csHitPoint;

    //bool hit = traceScreenSpaceRay1(
    //    eyePos,
    //    rayDir,
    //    ProjPixelMatrix,
    //    DepthTexture,
    //    texSize,
    //    0.5f, //  csZThickness, expected thickness of objects in scene (e.g. pillars)
    //    -0.5f, //camera.nearPlaneZ,
    //    2.0f, //stride,
    //    1 + float((int(clipPos.x) + int(clipPos.y)) & 1) * 0.5,
    //    225.0f, //maxSteps,
    //    80.0f, //maxRayTraceDistance,
    //    hitPixel,
    //    csHitPoint
    //);

    bool hit = traceScreenSpaceRay2(
        eyePos,
        rayDir,
        ProjPixelMatrix,
        DepthTexture,
        texSize,
        0.5f, //  csZThickness, expected thickness of objects in scene (e.g. pillars)
        -0.5f, //camera.nearPlaneZ,
        2.0f, //stride,
        1 + float((int(pIn.clipPos.x) + int(pIn.clipPos.y)) & 1) * 0.5,
        225.0f, //maxSteps,
        80.0f, //maxRayTraceDistance,
        hitPixel,
        csHitPoint
    );

    //bool hit = traceScreenSpaceRay(
    //    eyePos,
    //    rayDir,
    //    1 + float((int(clipPos.x) + int(clipPos.y)) & 1) * 0.5,
    //    12.29,
    //    -0.5f,
    //    texSize,
    //    22.59,
    //    30.0f,
    //    10,
    //    0.0f,
    //    hitPixel,
    //    csHitPoint
    //);

    //float jitter,
    //    float MaxDistance,
    //    float NearPlaneZ,
    //    float2 RTSize,
    //    float StrideZCutoff,
    //    float Stride,
    //    float MaxSteps,
    //    float ZThickness,
    //    // Pixel coordinates of the first intersection with the scene
    //    out float2 hitPixel,
    //    // Camera space location of the ray hit
    //    out float3 hitPoint);

    //float vis;
    //bool hit = ssr(texCoord, texSize, hitPixel, vis);

    if (hit)
    {
        hitPixel /= texSize;
        //hitPixel.y = 1.0f - hitPixel.y;
        return float4(ColorTexture.Sample(Sampler, hitPixel).rgb, 1.0);
    }
    else
    {
        //return float4(0, 0, 0, 0);
        return float4(ColorTexture.Sample(Sampler, texCoord).rgb, 0.0);
    }
}

#define vec2 float2
#define vec3 float3
#define vec4 float4
#define mat4x4 float4x4
#define sampler2D Texture2D

float texelFetch(sampler2D tex, int2 icoord)
{
    //return PositionTexture.Load(int3(icoord, 0)).z;
    return GetEyeDepth(tex, icoord);
}

// By Morgan McGuire and Michael Mara at Williams College 2014
// Released as open source under the BSD 2-Clause License
// http://opensource.org/licenses/BSD-2-Clause
#define point2 vec2
#define point3 vec3

float distanceSquared(vec2 a, vec2 b) { a -= b; return dot(a, a); }

//bool intersectsDepthBuffer(float z, float minZ, float maxZ, float ZThickness)
//{
//    /*
//    * Based on how far away from the camera the depth is,
//    * adding a bit of extra thickness can help improve some
//    * artifacts. Driving this value up too high can cause
//    * artifacts of its own.
//    */
//    //float depthScale = min(1.0f, z * StrideZCutoff);
//    //z += ZThickness + lerp(0.0f, 2.0f, depthScale);
//    return (maxZ >= z) && (minZ - ZThickness <= z);
//}


void swap(inout float f0, inout float f1)
{
    float temp = f0;
    f0 = f1;
    f1 = temp;
}

// Returns true if the ray hit something
bool traceScreenSpaceRay1(
    // Camera-space ray origin, which must be within the view volume
    point3 csOrig,

    // Unit length camera-space ray direction
    vec3 csDir,

    // A projection matrix that maps to pixel coordinates (not [-1, +1]
    // normalized device coordinates)
    mat4x4 proj,

    // The camera-space Z buffer (all negative values)
    sampler2D csZBuffer,

    // Dimensions of csZBuffer
    vec2 csZBufferSize,

    // Camera space thickness to ascribe to each pixel in the depth buffer
    float zThickness,

    // (Negative number)
    float nearPlaneZ,

    // Step in horizontal or vertical pixels between samples. This is a float
    // because integer math is slow on GPUs, but should be set to an integer >= 1
    float stride,

    // Number between 0 and 1 for how far to bump the ray in stride units
    // to conceal banding artifacts
    float jitter,

    // Maximum number of iterations. Higher gives better images but may be slow
    const float maxSteps,

    // Maximum camera-space distance to trace before returning a miss
    float maxDistance,

    // Pixel coordinates of the first intersection with the scene
    out point2 hitPixel,

    // Camera space location of the ray hit
    out point3 hitPoint) {

    // Clip to the near plane    
    float rayLength = ((csOrig.z + csDir.z * maxDistance) > nearPlaneZ) ?
        (nearPlaneZ - csOrig.z) / csDir.z : maxDistance;
    point3 csEndPoint = csOrig + csDir * rayLength;

    // Project into homogeneous clip space
    vec4 H0 = mul(vec4(csOrig, 1.0), proj);
    vec4 H1 = mul(vec4(csEndPoint, 1.0), proj);
    float k0 = 1.0 / H0.w, k1 = 1.0 / H1.w;

    // The interpolated homogeneous version of the camera-space points  
    point3 Q0 = csOrig * k0, Q1 = csEndPoint * k1;

    // Screen-space endpoints
    point2 P0 = H0.xy * k0, P1 = H1.xy * k1;

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    P1 += ((distanceSquared(P0, P1) < 0.0001) ? 0.01 : 0.0);
    vec2 delta = P1 - P0;

    // Permute so that the primary iteration is in x to collapse
    // all quadrant-specific DDA cases later
    bool permute = false;
    if (abs(delta.x) < abs(delta.y)) {
        // This is a more-vertical line
        permute = true; delta = delta.yx; P0 = P0.yx; P1 = P1.yx;
    }

    float stepDir = sign(delta.x);
    float invdx = stepDir / delta.x;

    // Track the derivatives of Q and k
    vec3  dQ = (Q1 - Q0) * invdx;
    float dk = (k1 - k0) * invdx;
    vec2  dP = vec2(stepDir, delta.y * invdx);

    // Scale derivatives by the desired pixel stride and then
    // offset the starting values by the jitter fraction
    float strideScale = 1.0f - min(1.0f, (csOrig.z) / 20.0f);
    //stride *= strideScale;
    dP *= stride; dQ *= stride; dk *= stride;
    P0 += dP * jitter; Q0 += dQ * jitter; k0 += dk * jitter;

    // Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, k from k0 to k1
    point3 Q = Q0;

    // Adjust end condition for iteration direction
    float  end = P1.x * stepDir;

    float k = k0, stepCount = 0.0, prevZMaxEstimate = csOrig.z;
    float rayZMin = prevZMaxEstimate, rayZMax = prevZMaxEstimate;
    float sceneZMax = rayZMax + 100;
    for (point2 P = P0;
        ((P.x * stepDir) <= end) && (stepCount < maxSteps) &&
        ((rayZMax < sceneZMax - zThickness) || (rayZMin > sceneZMax)) &&
        (sceneZMax != 0);
        P += dP, Q.z += dQ.z, k += dk, ++stepCount) {

        rayZMin = prevZMaxEstimate;
        rayZMax = (dQ.z * 0.5 + Q.z) / (dk * 0.5 + k);
        prevZMaxEstimate = rayZMax;
        if (rayZMin > rayZMax) {
            float t = rayZMin; rayZMin = rayZMax; rayZMax = t;
        }

        hitPixel = permute ? P.yx : P;
        // You may need hitPixel.y = csZBufferSize.y - hitPixel.y; here if your vertical axis
        // is different than ours in screen space
        //hitPixel.y = csZBufferSize.y - hitPixel.y;
        sceneZMax = texelFetch(csZBuffer, int2(hitPixel));
    }

    // Advance Q based on the number of steps
    Q.xy += dQ.xy * stepCount;
    hitPoint = Q * (1.0 / k);
    return (rayZMax >= sceneZMax - zThickness) && (rayZMin < sceneZMax);
}

#define Projection ProjMatrix

// Returns true if the ray hit something
bool traceScreenSpaceRay(
    // Camera-space ray origin, which must be within the view volume
    float3 csOrig,
    // Unit length camera-space ray direction
    float3 csDir,
    // Number between 0 and 1 for how far to bump the ray in stride units
    // to conceal banding artifacts. Not needed if stride == 1.
    float jitter,
    float MaxDistance,
    float NearPlaneZ,
    float2 RTSize,
    float StrideZCutoff,
    float Stride,
    float MaxSteps,
    float ZThickness,
    // Pixel coordinates of the first intersection with the scene
    out float2 hitPixel,
    // Camera space location of the ray hit
    out float3 hitPoint)
{
    // Clip to the near plane
    float rayLength = ((csOrig.z + csDir.z * MaxDistance) > NearPlaneZ) ?
        (NearPlaneZ - csOrig.z) / csDir.z : MaxDistance;
    float3 csEndPoint = csOrig + csDir * rayLength;

    // Project into homogeneous clip space
    float4 H0 = mul(Projection, float4(csOrig, 1.0f));
    float4 H1 = mul(Projection, float4(csEndPoint, 1.0f));

    float k0 = 1.0f / H0.w;
    float k1 = 1.0f / H1.w;

    // The interpolated homogeneous version of the camera-space points
    float3 Q0 = csOrig * k0;
    float3 Q1 = csEndPoint * k1;

    // Screen-space endpoints
    float2 P0 = H0.xy * k0;
    float2 P1 = H1.xy * k1;

    P0 = P0 * float2(0.5, -0.5) + float2(0.5, 0.5);
    P1 = P1 * float2(0.5, -0.5) + float2(0.5, 0.5);

    P0.xy *= RTSize.xy;
    P1.xy *= RTSize.xy;

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    P1 += (distanceSquared(P0, P1) < 0.0001f) ? float2(0.01f, 0.01f) : 0.0f;
    float2 delta = P1 - P0;

    // Permute so that the primary iteration is in x to collapse
    // all quadrant-specific DDA cases later
    bool permute = false;
    if (abs(delta.x) < abs(delta.y))
    {
        // This is a more-vertical line
        permute = true;
        delta = delta.yx;
        P0 = P0.yx;
        P1 = P1.yx;
    }

    float stepDir = sign(delta.x);
    float invdx = stepDir / delta.x;

    // Track the derivatives of Q and k
    float3 dQ = (Q1 - Q0) * invdx;
    float dk = (k1 - k0) * invdx;
    float2 dP = float2(stepDir, delta.y * invdx);

    // Scale derivatives by the desired pixel stride and then
    // offset the starting values by the jitter fraction
    float strideScale = 1.0f - min(1.0f, csOrig.z * StrideZCutoff);
    float stride = 1.0f + strideScale * Stride;
    dP *= stride;
    dQ *= stride;
    dk *= stride;

    P0 += dP * jitter;
    Q0 += dQ * jitter;
    k0 += dk * jitter;

    // Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, k from k0 to k1
    float4 PQk = float4(P0, Q0.z, k0);
    float4 dPQk = float4(dP, dQ.z, dk);
    float3 Q = Q0;

    // Adjust end condition for iteration direction
    float end = P1.x * stepDir;

    float stepCount = 0.0f;
    float prevZMaxEstimate = csOrig.z;
    float rayZMin = prevZMaxEstimate;
    float rayZMax = prevZMaxEstimate;
    float sceneZMax = rayZMax + 200.0f;

    for (;
        ((PQk.x * stepDir) <= end) && (stepCount < MaxSteps) &&
        //!intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax, ZThickness) &&
        ((rayZMax < sceneZMax - ZThickness) || (rayZMin > sceneZMax)) &&
        (sceneZMax != 0.0f);
        ++stepCount)
    {
        rayZMin = prevZMaxEstimate;
        rayZMax = (dPQk.z * 0.5f + PQk.z) / (dPQk.w * 0.5f + PQk.w);
        prevZMaxEstimate = rayZMax;

        if (rayZMin > rayZMax)
        {
            swap(rayZMin, rayZMax);
        }

        hitPixel = permute ? PQk.yx : PQk.xy;

        //sceneZMax = lineariseDepth(depthBuffer[hitPixel].r);
        sceneZMax = texelFetch(PositionTexture, int2(hitPixel));

        PQk += dPQk;
    }

    // Advance Q based on the number of steps
    Q.xy += dQ.xy * stepCount;
    hitPoint = Q * (1.0f / PQk.w);

    //return intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax, ZThickness);
    return (rayZMax >= sceneZMax - ZThickness) && (rayZMin < sceneZMax);
}

#define Vec2 float2
#define Vec3 float3
#define Vec4 float4
#define Point2 float2
#define Point3 float3
#define Point4 float4
#define Vector2 float2
#define Vector3 float3
#define Vector4  float4
#define Color3 float3


/**
  \file data-files/shader/screenSpaceRayTrace.glsl

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*

/**
\param csOrigin Camera-space ray origin, which must be
within the view volume and must have z < -0.01 and project within the valid screen rectangle

\param csDirection Unit length camera-space ray direction

\param projectToPixelMatrix A projection matrix that maps to pixel coordinates
    (not [-1, +1] normalized device coordinates). Usually g3d_ProjectToPixelMatrix or
    gbuffer_camera_projectToPixelMatrix.

\param csZBuffer The depth or camera-space Z buffer, depending on the value of \a csZBufferIsHyperbolic

\param csZBufferSize Dimensions of csZBuffer

\param csZThickness Camera space thickness to ascribe to each pixel in the depth buffer

\param csZBufferIsHyperbolic True if csZBuffer is an OpenGL depth buffer, false (faster) if
    csZBuffer contains (negative) "linear" camera space z values. Const so that the compiler can evaluate
    the branch based on it at compile time

\param clipInfo See G3D::Camera documentation

\param nearPlaneZ Negative number. Doesn't have to be THE actual near plane, just a reasonable value
    for clipping rays headed towards the camera

\param stride Step in horizontal or vertical pixels between samples. This is a float
    because integer math is slow on GPUs, but should be set to an integer >= 1

\param jitterFraction  Number between 0 and 1 for how far to bump the ray in stride units
    to conceal banding artifacts, plus the stride ray offset. It is recommended to set this
    to at least 1.0 to avoid self-intersection artifacts.
    Using 1 + float((int(gl_FragCoord.x) + int(gl_FragCoord.y.y)) & 1) * 0.5 gives a nice
    dither pattern when stride is > 1.0;

\param maxSteps Maximum number of iterations. Higher gives better images but may be slow

\param maxRayTraceDistance Maximum camera-space distance to trace before returning a miss

\param hitPixel Pixel coordinates of the first intersection with the scene

\param csHitPoint Camera space location of the ray hit

Single-layer

*/

bool traceScreenSpaceRay2(
    Point3          csOrigin,
    Vector3         csDirection,
    mat4x4          projectToPixelMatrix,
    sampler2D       csZBuffer,
    float2          csZBufferSize,
    float           csZThickness,
    float           nearPlaneZ,
    float			stride,
    float           jitterFraction,
    float           maxSteps,
    in float        maxRayTraceDistance,
    out Point2      hitPixel,
    out Point3		csHitPoint
) {
    // Clip ray to a near plane in 3D (doesn't have to be *the* near plane, although that would be a good idea)
    float rayLength = ((csOrigin.z + csDirection.z * maxRayTraceDistance) > nearPlaneZ) ?
    (nearPlaneZ - csOrigin.z) / csDirection.z :
    maxRayTraceDistance;
    Point3 csEndPoint = csDirection * rayLength + csOrigin;

    // Project into screen space
    Vector4 H0 = mul(Vector4(csOrigin, 1.0), projectToPixelMatrix);
    Vector4 H1 = mul(Vector4(csEndPoint, 1.0), projectToPixelMatrix);

    // There are a lot of divisions by w that can be turned into multiplications
    // at some minor precision loss...and we need to interpolate these 1/w values
    // anyway.
    //
    // Because the caller was required to clip to the near plane,
    // this homogeneous division (projecting from 4D to 2D) is guaranteed 
    // to succeed. 
    float k0 = 1.0 / H0.w;
    float k1 = 1.0 / H1.w;

    // Switch the original points to values that interpolate linearly in 2D
    Point3 Q0 = csOrigin * k0;
    Point3 Q1 = csEndPoint * k1;

    // Screen-space endpoints
    Point2 P0 = H0.xy * k0;
    Point2 P1 = H1.xy * k1;

    // [Optional clipping to frustum sides here]

    // Initialize to off screen
    hitPixel = Point2(-1.0, -1.0);

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
    P1 += ((distanceSquared(P0, P1) < 0.0001) ? 0.01 : 0.0);

    Vector2 delta = P1 - P0;

    // Permute so that the primary iteration is in x to reduce
    // large branches later
    bool permute = (abs(delta.x) < abs(delta.y));
    if (permute) {
    // More-vertical line. Create a permutation that swaps x and y in the output
    // by directly swizzling the inputs.
    delta = delta.yx;
    P1 = P1.yx;
    P0 = P0.yx;
    }

    // From now on, "x" is the primary iteration direction and "y" is the secondary one
    float stepDirection = sign(delta.x);
    float invdx = stepDirection / delta.x;
    Vector2 dP = Vector2(stepDirection, invdx * delta.y);

    // Track the derivatives of Q and k
    Vector3 dQ = (Q1 - Q0) * invdx;
    float   dk = (k1 - k0) * invdx;

    // Because we test 1/2 a texel forward along the ray, on the very last iteration
    // the interpolation can go past the end of the ray. Use these bounds to clamp it.
    float zMin = min(csEndPoint.z, csOrigin.z);
    float zMax = max(csEndPoint.z, csOrigin.z);

    // Scale derivatives by the desired pixel stride
    dP *= stride; dQ *= stride; dk *= stride;

    // Offset the starting values by the jitter fraction
    P0 += dP * jitterFraction; Q0 += dQ * jitterFraction; k0 += dk * jitterFraction;

    // Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, and k from k0 to k1
    Point3 Q = Q0;
    float  k = k0;

    // We track the ray depth at +/- 1/2 pixel to treat pixels as clip-space solid 
    // voxels. Because the depth at -1/2 for a given pixel will be the same as at 
    // +1/2 for the previous iteration, we actually only have to compute one value 
    // per iteration.
    float prevZMaxEstimate = csOrigin.z;
    float stepCount = 0.0;
    float rayZMax = prevZMaxEstimate, rayZMin = prevZMaxEstimate;
    float sceneZMax = rayZMax + 1e4;

    // P1.x is never modified after this point, so pre-scale it by 
    // the step direction for a signed comparison
    float end = P1.x * stepDirection;

    // We only advance the z field of Q in the inner loop, since
    // Q.xy is never used until after the loop terminates.

    Point2 P;
    for (P = P0;
    ((P.x * stepDirection) <= end) &&
    (stepCount < maxSteps) &&
    ((rayZMax < sceneZMax - csZThickness) ||
        (rayZMin > sceneZMax)) &&
    (sceneZMax != 0.0);
    P += dP, Q.z += dQ.z, k += dk, stepCount += 1.0) {

        // The depth range that the ray covers within this loop
        // iteration.  Assume that the ray is moving in increasing z
        // and swap if backwards.  Because one end of the interval is
        // shared between adjacent iterations, we track the previous
        // value and then swap as needed to ensure correct ordering
        rayZMin = prevZMaxEstimate;

        // Compute the value at 1/2 step into the future
        rayZMax = (dQ.z * 0.5 + Q.z) / (dk * 0.5 + k);
        rayZMax = clamp(rayZMax, zMin, zMax);
        prevZMaxEstimate = rayZMax;

        // Since we don't know if the ray is stepping forward or backward in depth,
        // maybe swap. Note that we preserve our original z "max" estimate first.
        if (rayZMin > rayZMax) { swap(rayZMin, rayZMax); }

        // Camera-space z of the background
        hitPixel = permute ? P.yx : P;
        sceneZMax = texelFetch(csZBuffer, int2(hitPixel));

    } // pixel on ray

    // Undo the last increment, which ran after the test variables
    // were set up.
    P -= dP; Q.z -= dQ.z; k -= dk; stepCount -= 1.0;

    bool hit = (rayZMax >= sceneZMax - csZThickness) && (rayZMin <= sceneZMax);

    // If using non-unit stride and we hit a depth surface...
    if ((stride > 1) && hit) {
        // Refine the hit point within the last large-stride step

        // Retreat one whole stride step from the previous loop so that
        // we can re-run that iteration at finer scale
        P -= dP; Q.z -= dQ.z; k -= dk; stepCount -= 1.0;

        // Take the derivatives back to single-pixel stride
        float invStride = 1.0 / stride;
        dP *= invStride; dQ.z *= invStride; dk *= invStride;

        // For this test, we don't bother checking thickness or passing the end, since we KNOW there will
        // be a hit point. As soon as
        // the ray passes behind an object, call it a hit. Advance (stride + 1) steps to fully check this 
        // interval (we could skip the very first iteration, but then we'd need identical code to prime the loop)
        float refinementStepCount = 0;

        // This is the current sample point's z-value, taken back to camera space
        prevZMaxEstimate = Q.z / k;
        rayZMin = prevZMaxEstimate;

        // Ensure that the FOR-loop test passes on the first iteration since we
        // won't have a valid value of sceneZMax to test.
        sceneZMax = rayZMin - 1e7;

        for (;
            (refinementStepCount <= stride * 1.4) &&
            (rayZMin > sceneZMax) && (sceneZMax != 0.0);
            P += dP, Q.z += dQ.z, k += dk, refinementStepCount += 1.0) {

            rayZMin = prevZMaxEstimate;

            // Compute the ray camera-space Z value at 1/2 fine step (pixel) into the future
            rayZMax = (dQ.z * 0.5 + Q.z) / (dk * 0.5 + k);
            rayZMax = clamp(rayZMax, zMin, zMax);

            prevZMaxEstimate = rayZMax;
            rayZMin = min(rayZMax, rayZMin);

            hitPixel = permute ? P.yx : P;
            sceneZMax = texelFetch(csZBuffer, int2(hitPixel));
        }

        // Undo the last increment, which happened after the test variables were set up
        Q.z -= dQ.z; refinementStepCount -= 1;

        // Count the refinement steps as fractions of the original stride. Save a register
        // by not retaining invStride until here
        stepCount += refinementStepCount / stride;
        //  debugColor = vec3(refinementStepCount / stride);
    } // refinement

    Q.xy += dQ.xy * stepCount;
    csHitPoint = Q * (1.0 / k);

    // Does the last point discovered represent a valid hit?
    return hit;
}

float2 projectTo01(in float4 viewPos)
{
    viewPos = mul(viewPos, ProjMatrix);
    viewPos /= viewPos.w;
    return (viewPos.xy * float2(0.5, -0.5) + 0.5);
}

bool ssr(in float2 texCoord, in float2 texSize, out float2 outCoord, out float outVisibility)
{
    float maxDistance = 80.0;
    float resolution = 0.8;
    int steps = 50;
    float thickness = 0.5;

    outCoord = texCoord;
    outVisibility = 0.0;

    int3 iCoord = int3(int2(texCoord * texSize), 0);
    float3 positionFrom = PositionTexture.Load(iCoord).xyz;

    float3 unitPositionFrom = normalize(positionFrom);
    float3 normal = normalize(NormalTexture.Load(iCoord).xyz);
    float3 pivot = normalize(reflect(unitPositionFrom, normal));

    float3 positionTo = positionFrom;

    float4 startView = float4(positionFrom + (pivot * 0), 1);
    float4 endView = float4(positionFrom + (pivot * maxDistance), 1);

    float2 startFrag = projectTo01(startView) * texSize;
    float2 endFrag = projectTo01(endView) * texSize;

    float2 frag = startFrag;
    float2 uv = frag / texSize;

    float deltaX = endFrag.x - startFrag.x;
    float deltaY = endFrag.y - startFrag.y;

    float useX = abs(deltaX) > abs(deltaY) ? 1 : 0;
    float delta = lerp(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0, 1);

    float2 increment = float2(deltaX, deltaY) / max(delta, 0.001);

    float search0 = 0.0;
    float search1 = 0.0;

    int hit0 = 0;

    startView.z *= -1.0f;
    endView.z *= -1.0f;

    float viewDistance = startView.z;
    float depth = thickness;

    float i = 0;
    for (i = 0; i < int(delta); ++i)
    {
        frag += increment;
        uv = frag / texSize;
        positionTo = PositionTexture.Load(int3(int2(uv * texSize), 0)).xyz;
        positionTo.z *= -1.0f;

        search1 = saturate(lerp((frag.y - startFrag.y) / deltaY, (frag.x - startFrag.x) / deltaX, useX));

        viewDistance = (startView.z * endView.z) / lerp(endView.z, startView.z, search1);
        depth = viewDistance - positionTo.z;

        if (depth > 0.0 && depth < thickness)
        {
            hit0 = 1;
            break;
        }
        else
        {
            search0 = search1;
        }

    }

    if (hit0 == 0)
        return false;

    search1 = search0 + ((search1 - search0) / 2.0);
    int hit1 = 0;

    [unroll]
    for (i = 0; i < steps; i++)
    {
        frag = lerp(startFrag, endFrag, search1);
        uv = frag / texSize;
        positionTo = PositionTexture.Load(int3(int2(uv * texSize), 0)).xyz;
        positionTo.z *= -1.0f;

        viewDistance = (startView.z * endView.z) / lerp(endView.z, startView.z, search1);
        depth = viewDistance - positionTo.z;

        if (depth > 0.0 && depth < thickness)
        {
            hit1 = 1;
            search1 = search0 + ((search1 - search0) / 2.0);
        }
        else
        {
            float temp = search1;
            search1 = search1 + ((search1 - search0) / 2.0);
            search0 = temp;
        }
    }

    if (hit1 == 0)
        return false;

    if (uv.x < 0.0 || uv.x > 1 || uv.y < 0.0 || uv.y > 1)
        return false;

    outCoord = uv * texSize;
    outVisibility = 1.0;
    return true;
}