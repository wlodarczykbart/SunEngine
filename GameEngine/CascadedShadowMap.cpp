#include "Camera.h"
#include "FilePathMgr.h"
#include "BaseShader.h"
#include "CascadedShadowMap.h"

namespace SunEngine
{
	CascadedShadowMap::CascadedShadowMap()
	{
	}

	CascadedShadowMap::~CascadedShadowMap()
	{
	}

	void CreateFrustumPointsFromCascadeInterval(float fCascadeIntervalBegin,
		float fCascadeIntervalEnd,
		glm::mat4 vInvProjection,
		glm::vec4* pvCornerPointsWorld);

	void ComputeNearAndFar(float& fNearPlane,
		float& fFarPlane,
		glm::vec4 vLightCameraOrthographicMin,
		glm::vec4 vLightCameraOrthographicMax,
		glm::vec4* pvPointsInCameraView);

	void CascadedShadowMap::Update(const UpdateInfo& updateInfo, glm::mat4& lightViewMatrix, Camera* pShadowCameras, float* pShadowFrustumParitions)
	{
		const CameraComponentData* pCameraData = updateInfo.pCameraData;
		const Camera* pCamera = pCameraData->C()->As<Camera>();
		float farClip = pCamera->GetFarZ();
		float nearClip = pCamera->GetNearZ();

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		float clipRange = farClip - nearClip;
		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		uint cacadeCount = EngineInfo::GetRenderer().CascadeShadowMapSplits();
		float cascadePartitionsZeroToOne[ShadowBufferData::MAX_CASCADE_SPLITS] = {};
		float cascadePartitionsMax = 0.0f;

		for (uint i = 0; i < cacadeCount; i++)
		{
			float p = (i + 1) / static_cast<float>(cacadeCount);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = updateInfo.cascadeSplitLambda * (log - uniform) + uniform;
			cascadePartitionsZeroToOne[i] = ((d - nearClip)/* / clipRange*/);
			cascadePartitionsMax = cascadePartitionsZeroToOne[i];
		}

		const glm::mat4& matInverseProjectionCamera = pCamera->GetInvProj();
		const glm::mat4& matInverseViewCamera = pCameraData->GetInvView();
		lightViewMatrix = glm::lookAt(updateInfo.lightPos, Vec3::Zero, Vec3::Up);

		//NOTE: not using the built in corner function because the csm code I am basing this off of expects the corners in a certain order to generate correct triangles
		glm::vec4 vSceneAABBPointsLightSpace[8] =
		{
			glm::vec4(updateInfo.sceneBounds.Min.x, updateInfo.sceneBounds.Min.y, updateInfo.sceneBounds.Max.z, 1.0f),
			glm::vec4(updateInfo.sceneBounds.Max.x, updateInfo.sceneBounds.Min.y, updateInfo.sceneBounds.Max.z, 1.0f),
			glm::vec4(updateInfo.sceneBounds.Max.x, updateInfo.sceneBounds.Max.y, updateInfo.sceneBounds.Max.z, 1.0f),
			glm::vec4(updateInfo.sceneBounds.Min.x, updateInfo.sceneBounds.Max.y, updateInfo.sceneBounds.Max.z, 1.0f),
			glm::vec4(updateInfo.sceneBounds.Min.x, updateInfo.sceneBounds.Min.y, updateInfo.sceneBounds.Min.z, 1.0f),
			glm::vec4(updateInfo.sceneBounds.Max.x, updateInfo.sceneBounds.Min.y, updateInfo.sceneBounds.Min.z, 1.0f),
			glm::vec4(updateInfo.sceneBounds.Max.x, updateInfo.sceneBounds.Max.y, updateInfo.sceneBounds.Min.z, 1.0f),
			glm::vec4(updateInfo.sceneBounds.Min.x, updateInfo.sceneBounds.Max.y, updateInfo.sceneBounds.Min.z, 1.0f),
		};

		for (int index = 0; index < 8; ++index)
		{
			vSceneAABBPointsLightSpace[index] = lightViewMatrix * vSceneAABBPointsLightSpace[index];
		}

		float fFrustumIntervalBegin, fFrustumIntervalEnd;
		glm::vec4 vLightCameraOrthographicMin;  // light space frustrum aabb 
		glm::vec4 vLightCameraOrthographicMax;
		float fCameraNearFarRange = farClip - nearClip;

		glm::vec4 vWorldUnitsPerTexel = Vec4::Zero;

		uint cascadeFitMode = updateInfo.cascadeFitMode;
		uint nearFarFitMode = updateInfo.nearFarFitMode;

		// We loop over the cascades to calculate the orthographic projection for each cascade.
		for (uint iCascadeIndex = 0; iCascadeIndex < cacadeCount; ++iCascadeIndex)
		{
			// Calculate the interval of the View Frustum that this cascade covers. We measure the interval 
			// the cascade covers as a Min and Max distance along the Z Axis.
			if (cascadeFitMode == FIT_TO_CASCADES)
			{
				// Because we want to fit the orthogrpahic projection tightly around the Cascade, we set the Mimiumum cascade 
				// value to the previous Frustum end Interval
				if (iCascadeIndex == 0) fFrustumIntervalBegin = 0.0f;
				else fFrustumIntervalBegin = cascadePartitionsZeroToOne[iCascadeIndex - 1];
			}
			else
			{
				// In the FIT_TO_SCENE technique the Cascades overlap eachother.  In other words, interval 1 is coverd by
				// cascades 1 to 8, interval 2 is covered by cascades 2 to 8 and so forth.
				fFrustumIntervalBegin = 0.0f;
			}

			// Scale the intervals between 0 and 1. They are now percentages that we can scale with.
			fFrustumIntervalEnd = cascadePartitionsZeroToOne[iCascadeIndex];
			fFrustumIntervalBegin /= cascadePartitionsMax;
			fFrustumIntervalEnd /= cascadePartitionsMax;
			fFrustumIntervalBegin = fFrustumIntervalBegin * fCameraNearFarRange;
			fFrustumIntervalEnd = fFrustumIntervalEnd * fCameraNearFarRange;
			glm::vec4 vFrustumPoints[8];

			// This function takes the began and end intervals along with the projection matrix and returns the 8
			// points that repreresent the cascade Interval
			CreateFrustumPointsFromCascadeInterval(fFrustumIntervalBegin, fFrustumIntervalEnd,
				matInverseProjectionCamera, vFrustumPoints);

			vLightCameraOrthographicMin = Vec4::Max;
			vLightCameraOrthographicMax = Vec4::Min;

			glm::vec4 vTempTranslatedCornerPoint;
			// This next section of code calculates the min and max values for the orthographic projection.
			for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
			{
				// Transform the frustum from camera view space to world space.
				vFrustumPoints[icpIndex] = matInverseViewCamera * vFrustumPoints[icpIndex];
				// Transform the point from world space to Light Camera Space.
				vTempTranslatedCornerPoint = lightViewMatrix * vFrustumPoints[icpIndex];
				// Find the closest point.
				vLightCameraOrthographicMin = glm::min(vTempTranslatedCornerPoint, vLightCameraOrthographicMin);
				vLightCameraOrthographicMax = glm::max(vTempTranslatedCornerPoint, vLightCameraOrthographicMax);
			}

			// This code removes the shimmering effect along the edges of shadows due to
			// the light changing to fit the camera.
			if (cascadeFitMode == FIT_TO_SCENE)
			{
				// Fit the ortho projection to the cascades far plane and a near plane of zero. 
				// Pad the projection to be the size of the diagonal of the Frustum partition. 
				// 
				// To do this, we pad the ortho transform so that it is always big enough to cover 
				// the entire camera view frustum.
				glm::vec4 vDiagonal = vFrustumPoints[0] - vFrustumPoints[6];
				vDiagonal = glm::vec4(glm::length(glm::vec3(vDiagonal)));

				// The bound is the length of the diagonal of the frustum interval.
				float fCascadeBound = vDiagonal.x;

				// The offset calculated will pad the ortho projection so that it is always the same size 
				// and big enough to cover the entire cascade interval.
				glm::vec4 vBoarderOffset = (vDiagonal -
					(vLightCameraOrthographicMax - vLightCameraOrthographicMin))
					* Vec4::Half;
				// Set the Z and W components to zero.
				vBoarderOffset *= glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);

				// Add the offsets to the projection.
				vLightCameraOrthographicMax += vBoarderOffset;
				vLightCameraOrthographicMin -= vBoarderOffset;

				// The world units per texel are used to snap the shadow the orthographic projection
				// to texel sized increments.  This keeps the edges of the shadows from shimmering.
				float fWorldUnitsPerTexel = fCascadeBound / (float)EngineInfo::GetRenderer().CascadeShadowMapResolution();
				vWorldUnitsPerTexel = glm::vec4(fWorldUnitsPerTexel, fWorldUnitsPerTexel, 0.0f, 0.0f);


			}
			else if (cascadeFitMode == FIT_TO_CASCADES)
			{

				// We calculate a looser bound based on the size of the PCF blur.  This ensures us that we're 
				// sampling within the correct map.
				float fScaleDuetoBlureAMT = ((float)(EngineInfo::GetRenderer().CascadeShadowMapPCFBlurSize() * 2 + 1)
					/ (float)EngineInfo::GetRenderer().CascadeShadowMapResolution());
				glm::vec4 vScaleDuetoBlureAMT = { fScaleDuetoBlureAMT, fScaleDuetoBlureAMT, 0.0f, 0.0f };


				float fNormalizeByBufferSize = 1.0f / (float)EngineInfo::GetRenderer().CascadeShadowMapResolution();
				glm::vec4 vNormalizeByBufferSize = glm::vec4(fNormalizeByBufferSize, fNormalizeByBufferSize, 0.0f, 0.0f);

				// We calculate the offsets as a percentage of the bound.
				glm::vec4 vBoarderOffset = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
				vBoarderOffset *= Vec4::Half;
				vBoarderOffset *= vScaleDuetoBlureAMT;
				vLightCameraOrthographicMax += vBoarderOffset;
				vLightCameraOrthographicMin -= vBoarderOffset;

				// The world units per texel are used to snap  the orthographic projection
				// to texel sized increments.  
				// Because we're fitting tighly to the cascades, the shimmering shadow edges will still be present when the 
				// camera rotates.  However, when zooming in or strafing the shadow edge will not shimmer.
				vWorldUnitsPerTexel = vLightCameraOrthographicMax - vLightCameraOrthographicMin;
				vWorldUnitsPerTexel *= vNormalizeByBufferSize;

			}
			float fLightCameraOrthographicMinZ = vLightCameraOrthographicMin.x;


			//if (m_bMoveLightTexelSize)
			{

				// We snape the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
				// This is a matter of integer dividing by the world space size of a texel
				vLightCameraOrthographicMin /= vWorldUnitsPerTexel;
				vLightCameraOrthographicMin = glm::floor(vLightCameraOrthographicMin);
				vLightCameraOrthographicMin *= vWorldUnitsPerTexel;

				vLightCameraOrthographicMax /= vWorldUnitsPerTexel;
				vLightCameraOrthographicMax = glm::floor(vLightCameraOrthographicMax);
				vLightCameraOrthographicMax *= vWorldUnitsPerTexel;

			}

			//These are the unconfigured near and far plane values.  They are purposly awful to show 
			// how important calculating accurate near and far planes is.
			float fNearPlane = 0.0f;
			float fFarPlane = 10000.0f;

			if (nearFarFitMode == FIT_NEARFAR_AABB)
			{

				glm::vec4 vLightSpaceSceneAABBminValue = Vec4::Max;  // world space scene aabb 
				glm::vec4 vLightSpaceSceneAABBmaxValue = Vec4::Min;
				// We calculate the min and max vectors of the scene in light space. The min and max "Z" values of the  
				// light space AABB can be used for the near and far plane. This is easier than intersecting the scene with the AABB
				// and in some cases provides similar results.
				for (int index = 0; index < 8; ++index)
				{
					vLightSpaceSceneAABBminValue = glm::min(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBminValue);
					vLightSpaceSceneAABBmaxValue = glm::max(vSceneAABBPointsLightSpace[index], vLightSpaceSceneAABBmaxValue);
				}

				// The min and max z values are the near and far planes.
				fNearPlane = vLightSpaceSceneAABBminValue.z;
				fFarPlane = vLightSpaceSceneAABBmaxValue.z;
			}
			else if (nearFarFitMode == FIT_NEARFAR_SCENE_AABB
				|| nearFarFitMode == FIT_NEARFAR_PANCAKING)
			{
				// By intersecting the light frustum with the scene AABB we can get a tighter bound on the near and far plane.
				ComputeNearAndFar(fNearPlane, fFarPlane, vLightCameraOrthographicMin,
					vLightCameraOrthographicMax, vSceneAABBPointsLightSpace);
				if (nearFarFitMode == FIT_NEARFAR_PANCAKING)
				{
					if (fLightCameraOrthographicMinZ > fNearPlane)
					{
						fNearPlane = fLightCameraOrthographicMinZ;
					}
				}
			}
			// Create the orthographic projection for this cascade.
			pShadowCameras[iCascadeIndex].SetOrthoProjection(vLightCameraOrthographicMin.x, vLightCameraOrthographicMax.x,
				vLightCameraOrthographicMin.y, vLightCameraOrthographicMax.y,
				fNearPlane, fFarPlane);
			pShadowFrustumParitions[iCascadeIndex] = fFrustumIntervalEnd;
		}
	}

	//--------------------------------------------------------------------------------------
	// This function takes the camera's projection matrix and returns the 8
	// points that make up a view frustum.
	// The frustum is scaled to fit within the Begin and End interval paramaters.
	//--------------------------------------------------------------------------------------
	void CreateFrustumPointsFromCascadeInterval(float fCascadeIntervalBegin,
		float fCascadeIntervalEnd,
		glm::mat4 vInvProjection,
		glm::vec4* pvCornerPointsWorld)
	{
		float nearZ = -1.0f * 1.0f;
		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, nearZ),
			glm::vec3(1.0f,  1.0f, nearZ),
			glm::vec3(1.0f, -1.0f, nearZ),
			glm::vec3(-1.0f, -1.0f, nearZ),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		for (uint32_t i = 0; i < 8; i++) 
		{
			glm::vec4 invCorner = vInvProjection * glm::vec4(frustumCorners[i], 1.0f);
			frustumCorners[i] = invCorner / invCorner.w;
		}

		//This range might not be the EXACT value you would get subtracting far - near due to precision 
		float nearFarRange = glm::abs((frustumCorners[4] - frustumCorners[0]).z);
		fCascadeIntervalBegin /= nearFarRange;
		fCascadeIntervalEnd /= nearFarRange;

		for (uint32_t i = 0; i < 4; i++) 
		{
			glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
			frustumCorners[i + 4] = frustumCorners[i] + (dist * fCascadeIntervalEnd);
			frustumCorners[i] = frustumCorners[i] + (dist * fCascadeIntervalBegin);
		}

		for (uint32_t i = 0; i < 8; i++) 
		{
			pvCornerPointsWorld[i] = glm::vec4(frustumCorners[i], 1.0f);
		}
	}

	//--------------------------------------------------------------------------------------
	// Computing an accurate near and flar plane will decrease surface acne and Peter-panning.
	// Surface acne is the term for erroneous self shadowing.  Peter-panning is the effect where
	// shadows disappear near the base of an object.
	// As offsets are generally used with PCF filtering due self shadowing issues, computing the
	// correct near and far planes becomes even more important.
	// This concept is not complicated, but the intersection code is.
	//--------------------------------------------------------------------------------------
	void ComputeNearAndFar(float& fNearPlane,
		float& fFarPlane,
		glm::vec4 vLightCameraOrthographicMin,
		glm::vec4 vLightCameraOrthographicMax,
		glm::vec4* pvPointsInCameraView)
	{

		//--------------------------------------------------------------------------------------
		// Used to compute an intersection of the orthographic projection and the Scene AABB
		//--------------------------------------------------------------------------------------
		struct Triangle
		{
			glm::vec4 pt[3];
			bool culled;
		};

		// Initialize the near and far planes
		fNearPlane = FLT_MAX;
		fFarPlane = -FLT_MAX;

		Triangle triangleList[16];
		int iTriangleCnt = 1;

		triangleList[0].pt[0] = pvPointsInCameraView[0];
		triangleList[0].pt[1] = pvPointsInCameraView[1];
		triangleList[0].pt[2] = pvPointsInCameraView[2];
		triangleList[0].culled = false;

		// These are the indices used to tesselate an AABB into a list of triangles.
		static const int iAABBTriIndexes[] =
		{
			0,1,2,  1,2,3,
			4,5,6,  5,6,7,
			0,2,4,  2,4,6,
			1,3,5,  3,5,7,
			0,1,4,  1,4,5,
			2,3,6,  3,6,7
		};

		int iPointPassesCollision[3];

		// At a high level: 
		// 1. Iterate over all 12 triangles of the AABB.  
		// 2. Clip the triangles against each plane. Create new triangles as needed.
		// 3. Find the min and max z values as the near and far plane.

		//This is easier because the triangles are in camera spacing making the collisions tests simple comparisions.

		float fLightCameraOrthographicMinX = vLightCameraOrthographicMin.x;
		float fLightCameraOrthographicMaxX = vLightCameraOrthographicMax.x;
		float fLightCameraOrthographicMinY = vLightCameraOrthographicMin.y;
		float fLightCameraOrthographicMaxY = vLightCameraOrthographicMax.y;

		for (int AABBTriIter = 0; AABBTriIter < 12; ++AABBTriIter)
		{

			triangleList[0].pt[0] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 0]];
			triangleList[0].pt[1] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 1]];
			triangleList[0].pt[2] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 2]];
			iTriangleCnt = 1;
			triangleList[0].culled = false;

			// Clip each invidual triangle against the 4 frustums.  When ever a triangle is clipped into new triangles, 
			//add them to the list.
			for (int frustumPlaneIter = 0; frustumPlaneIter < 4; ++frustumPlaneIter)
			{

				float fEdge;
				int iComponent;

				if (frustumPlaneIter == 0)
				{
					fEdge = fLightCameraOrthographicMinX; // todo make float temp
					iComponent = 0;
				}
				else if (frustumPlaneIter == 1)
				{
					fEdge = fLightCameraOrthographicMaxX;
					iComponent = 0;
				}
				else if (frustumPlaneIter == 2)
				{
					fEdge = fLightCameraOrthographicMinY;
					iComponent = 1;
				}
				else
				{
					fEdge = fLightCameraOrthographicMaxY;
					iComponent = 1;
				}

				for (int triIter = 0; triIter < iTriangleCnt; ++triIter)
				{
					// We don't delete triangles, so we skip those that have been culled.
					if (!triangleList[triIter].culled)
					{
						int iInsideVertCount = 0;
						glm::vec4 tempOrder;
						// Test against the correct frustum plane.
						// This could be written more compactly, but it would be harder to understand.

						if (frustumPlaneIter == 0)
						{
							for (int triPtIter = 0; triPtIter < 3; ++triPtIter)
							{
								if (triangleList[triIter].pt[triPtIter].x >
									vLightCameraOrthographicMin.x)
								{
									iPointPassesCollision[triPtIter] = 1;
								}
								else
								{
									iPointPassesCollision[triPtIter] = 0;
								}
								iInsideVertCount += iPointPassesCollision[triPtIter];
							}
						}
						else if (frustumPlaneIter == 1)
						{
							for (int triPtIter = 0; triPtIter < 3; ++triPtIter)
							{
								if (triangleList[triIter].pt[triPtIter].x <
									vLightCameraOrthographicMax.x)
								{
									iPointPassesCollision[triPtIter] = 1;
								}
								else
								{
									iPointPassesCollision[triPtIter] = 0;
								}
								iInsideVertCount += iPointPassesCollision[triPtIter];
							}
						}
						else if (frustumPlaneIter == 2)
						{
							for (int triPtIter = 0; triPtIter < 3; ++triPtIter)
							{
								if (triangleList[triIter].pt[triPtIter].y >
									vLightCameraOrthographicMin.y)
								{
									iPointPassesCollision[triPtIter] = 1;
								}
								else
								{
									iPointPassesCollision[triPtIter] = 0;
								}
								iInsideVertCount += iPointPassesCollision[triPtIter];
							}
						}
						else
						{
							for (int triPtIter = 0; triPtIter < 3; ++triPtIter)
							{
								if (triangleList[triIter].pt[triPtIter].y <
									vLightCameraOrthographicMax.y)
								{
									iPointPassesCollision[triPtIter] = 1;
								}
								else
								{
									iPointPassesCollision[triPtIter] = 0;
								}
								iInsideVertCount += iPointPassesCollision[triPtIter];
							}
						}

						// Move the points that pass the frustum test to the begining of the array.
						if (iPointPassesCollision[1] && !iPointPassesCollision[0])
						{
							tempOrder = triangleList[triIter].pt[0];
							triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
							triangleList[triIter].pt[1] = tempOrder;
							iPointPassesCollision[0] = true;
							iPointPassesCollision[1] = false;
						}
						if (iPointPassesCollision[2] && !iPointPassesCollision[1])
						{
							tempOrder = triangleList[triIter].pt[1];
							triangleList[triIter].pt[1] = triangleList[triIter].pt[2];
							triangleList[triIter].pt[2] = tempOrder;
							iPointPassesCollision[1] = true;
							iPointPassesCollision[2] = false;
						}
						if (iPointPassesCollision[1] && !iPointPassesCollision[0])
						{
							tempOrder = triangleList[triIter].pt[0];
							triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
							triangleList[triIter].pt[1] = tempOrder;
							iPointPassesCollision[0] = true;
							iPointPassesCollision[1] = false;
						}

						if (iInsideVertCount == 0)
						{ // All points failed. We're done,  
							triangleList[triIter].culled = true;
						}
						else if (iInsideVertCount == 1)
						{// One point passed. Clip the triangle against the Frustum plane
							triangleList[triIter].culled = false;

							// 
							glm::vec4 vVert0ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[0];
							glm::vec4 vVert0ToVert2 = triangleList[triIter].pt[2] - triangleList[triIter].pt[0];

							// Find the collision ratio.
							float fHitPointTimeRatio = fEdge - triangleList[triIter].pt[0][iComponent];
							// Calculate the distance along the vector as ratio of the hit ratio to the component.
							float fDistanceAlongVector01 = fHitPointTimeRatio / vVert0ToVert1[iComponent];
							float fDistanceAlongVector02 = fHitPointTimeRatio / vVert0ToVert2[iComponent];
							// Add the point plus a percentage of the vector.
							vVert0ToVert1 *= fDistanceAlongVector01;
							vVert0ToVert1 += triangleList[triIter].pt[0];
							vVert0ToVert2 *= fDistanceAlongVector02;
							vVert0ToVert2 += triangleList[triIter].pt[0];

							triangleList[triIter].pt[1] = vVert0ToVert2;
							triangleList[triIter].pt[2] = vVert0ToVert1;

						}
						else if (iInsideVertCount == 2)
						{ // 2 in  // tesselate into 2 triangles


							// Copy the triangle\(if it exists) after the current triangle out of
							// the way so we can override it with the new triangle we're inserting.
							triangleList[iTriangleCnt] = triangleList[triIter + 1];

							triangleList[triIter].culled = false;
							triangleList[triIter + 1].culled = false;

							// Get the vector from the outside point into the 2 inside points.
							glm::vec4 vVert2ToVert0 = triangleList[triIter].pt[0] - triangleList[triIter].pt[2];
							glm::vec4 vVert2ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[2];

							// Get the hit point ratio.
							float fHitPointTime_2_0 = fEdge - triangleList[triIter].pt[2][iComponent];
							float fDistanceAlongVector_2_0 = fHitPointTime_2_0 / vVert2ToVert0[iComponent];
							// Calcaulte the new vert by adding the percentage of the vector plus point 2.
							vVert2ToVert0 *= fDistanceAlongVector_2_0;
							vVert2ToVert0 += triangleList[triIter].pt[2];

							// Add a new triangle.
							triangleList[triIter + 1].pt[0] = triangleList[triIter].pt[0];
							triangleList[triIter + 1].pt[1] = triangleList[triIter].pt[1];
							triangleList[triIter + 1].pt[2] = vVert2ToVert0;

							//Get the hit point ratio.
							float fHitPointTime_2_1 = fEdge - triangleList[triIter].pt[2][iComponent];
							float fDistanceAlongVector_2_1 = fHitPointTime_2_1 / vVert2ToVert1[iComponent];
							vVert2ToVert1 *= fDistanceAlongVector_2_1;
							vVert2ToVert1 += triangleList[triIter].pt[2];
							triangleList[triIter].pt[0] = triangleList[triIter + 1].pt[1];
							triangleList[triIter].pt[1] = triangleList[triIter + 1].pt[2];
							triangleList[triIter].pt[2] = vVert2ToVert1;
							// Cncrement triangle count and skip the triangle we just inserted.
							++iTriangleCnt;
							++triIter;


						}
						else
						{ // all in
							triangleList[triIter].culled = false;

						}
					}// end if !culled loop            
				}
			}
			for (int index = 0; index < iTriangleCnt; ++index)
			{
				if (!triangleList[index].culled)
				{
					// Set the near and far plan and the min and max z values respectivly.
					for (int vertind = 0; vertind < 3; ++vertind)
					{
						float fTriangleCoordZ = -triangleList[index].pt[vertind].z;
						if (fNearPlane > fTriangleCoordZ)
						{
							fNearPlane = fTriangleCoordZ;
						}
						if (fFarPlane < fTriangleCoordZ)
						{
							fFarPlane = fTriangleCoordZ;
						}
					}
				}
			}
		}

	}
}