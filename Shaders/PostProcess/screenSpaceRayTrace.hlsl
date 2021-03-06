#define vec2 float2
#define vec3 float3
#define vec4 float4
#define mat4x4 float4x4
#define sampler2D Texture2D

float texelFetch(sampler2D tex, int2 icoord)
{
    //return GetEyeDepth(tex, icoord);
	return tex.Load(int3(icoord, 0)).z;
}

// By Morgan McGuire and Michael Mara at Williams College 2014
// Released as open source under the BSD 2-Clause License
// http://opensource.org/licenses/BSD-2-Clause
#define point2 vec2
#define point3 vec3

float distanceSquared(vec2 a, vec2 b) { a -= b; return dot(a, a); }

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
    const float maxDistance,

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

void swap(inout float f0, inout float f1)
{
    float temp = f0;
    f0 = f1;
    f1 = temp;
}

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
    const float     maxSteps,
    const float     maxRayTraceDistance,
    out Point2      hitPixel,
    out Point3		csHitPoint,
	out float 		iterations
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
	iterations = 0;

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

		iterations += 1.0;
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