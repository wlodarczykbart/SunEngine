#pragma once

#include "Types.h"
#include "glm/glm.hpp"

#define RAY_TRI_TOL 0.0001f
#define RAY_PLANE_TOL 0.0001f

namespace SunEngine
{
	struct Ray
	{
		glm::vec3 Origin;
		glm::vec3 Direction;

		void Transform(const glm::mat4& mtx)
		{
			Origin = glm::vec3(mtx * glm::vec4(Origin, 1.0f));
			Direction = glm::vec3(mtx * glm::vec4(Direction, 0.0f));
		}
	};

	struct Sphere
	{
		glm::vec3 Center;
		float Radius;
	};

	struct AABB
	{
		AABB()
		{
			Reset();
		}

		AABB(const glm::vec3& min, const glm::vec3& max)
		{
			Min = min;
			Max = max;
		}

		void Reset()
		{
			Min = glm::vec3(FLT_MAX);
			Max = glm::vec3(-FLT_MAX);
		}

		void Expand(const glm::vec3& point)
		{
			Min = glm::min(Min, point);
			Max = glm::max(Max, point);
		}

		void Expand(const AABB& box)
		{
			Expand(box.Min);
			Expand(box.Max);
		}

		void Transform(const glm::mat4& mtx)
		{
			glm::vec4 corners[8];
			corners[0] = glm::vec4(Min, 1.0f);
			corners[1] = glm::vec4(Max, 1.0f);

			corners[2] = glm::vec4(Min.x, Min.y, Max.z, 1.0f);
			corners[3] = glm::vec4(Min.x, Max.y, Max.z, 1.0f);
			corners[4] = glm::vec4(Max.x, Min.y, Max.z, 1.0f);

			corners[5] = glm::vec4(Max.x, Max.y, Min.z, 1.0f);
			corners[6] = glm::vec4(Min.x, Max.y, Min.z, 1.0f);
			corners[7] = glm::vec4(Max.x, Min.y, Min.z, 1.0f);

			Reset();
			for (uint i = 0; i < 8; i++)
				Expand(mtx * corners[i]);
		}

		glm::vec3 GetExtent() const
		{
			return (Max - Min) * 0.5f;
		}

		glm::vec3 GetCenter() const
		{
			return Min + GetExtent();
		}

		bool operator == (const AABB& rhs) const { return Min == rhs.Min && Max == rhs.Max; }
		bool operator != (const AABB& rhs) const { return !( Min == rhs.Min && Max == rhs.Max); }

		glm::vec3 Min;
		glm::vec3 Max;
	};

	inline bool RayPlaneIntersect(const Ray& ray, const glm::vec3& p, const glm::vec3& n, float& t)
	{
		//o + td
		//(o + td - p) . n = 0
		//o - p + td . n = 0
		//(o - p).n + td.n = 0
		//t = (p - o).n / d.n

		float dn = glm::dot(ray.Direction, n);
		if (fabsf(dn) < RAY_PLANE_TOL) return false;

		t = glm::dot(p - ray.Origin, n) / dn;
		return t >= 0.0f;
	}

	inline bool RaySphereIntersect(const Ray& ray, const glm::vec3& center, float radius)
	{
		glm::vec3 rayToCenter = center - ray.Origin;

		float l = glm::length(ray.Direction);
		float b = glm::dot(ray.Direction, rayToCenter) / l; //ray might not be normalized if it was moved to model space, could be way to do this without the call to length
		float b2 = b * b;
		float r2 = radius * radius;

		//ray is facing other direction and the ray in not inside the sphere, exit
		if(b < 0.0f && b2 > r2) return false;

		float c2 = glm::dot(rayToCenter, rayToCenter);
		float a2 = c2 - b2;

		//ray travels outside the sphere radius
		if (a2 > r2) return false;

		return true;
	}

	inline bool RayAABBIntersect(const Ray& ray, const glm::vec3& min, const glm::vec3& max)
	{
		//project the ray onto each axis at min/max and perform a series of logical tests which can be validated
		//by drawing a 2d box and tracking the "farthest entries" and "nearest exits" at each projection axis then extend the case to 3d

		float tmin = (min.x - ray.Origin.x) / ray.Direction.x;
		float tmax = (max.x - ray.Origin.x) / ray.Direction.x;

		if (tmin > tmax) std::swap(tmin, tmax);

		float tminy = (min.y - ray.Origin.y) / ray.Direction.y;
		float tmaxy = (max.y - ray.Origin.y) / ray.Direction.y;

		if (tminy > tmaxy) std::swap(tminy, tmaxy);

		if (tminy > tmax || tmin > tmaxy) return false;

		if (tminy > tmin)
			tmin = tminy;

		if (tmaxy < tmax)
			tmax = tmaxy;

		float tminz = (min.z - ray.Origin.z) / ray.Direction.z;
		float tmaxz = (max.z - ray.Origin.z) / ray.Direction.z;

		if (tminz > tmaxz) std::swap(tminz, tmaxz);

		if (tminz > tmax || tmin > tmaxz) return false;

		return true;
	}

	inline bool RayTriangleIntersect(const Ray& ray, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, float& tMinDist, float weights[3])
	{
#if 1
		//p = p0a + p1b + p2c
		//p = p0(1 - b - c) + p1b + p2c
		//(p - p0) = (p1 - p0)b + (p2 - p0)c
		//(p - p0)x(p2 - p0) = (p1 - p0)x(p2 - p0)b
		//(p - p0)x(p2 - p0).(p1 - p0)x(p2 - p0) = (p1 - p0)x(p2 - p0).(p1 - p0)x(p2 - p0)b
		//(p - p0)x(p2 - p0).(p1 - p0)x(p2 - p0) / (p1 - p0)x(p2 - p0).(p1 - p0)x(p2 - p0) = b

		glm::vec3 p1p0 = p1 - p0;
		glm::vec3 p2p0 = p2 - p0;

		glm::vec3 n = glm::cross(p1p0, p2p0);

		float t = 0.0f;
		if (!RayPlaneIntersect(ray, p0, n, t))
			return false;

		if (t > tMinDist)
			return false;

		glm::vec3 p = ray.Origin + ray.Direction * t;
		glm::vec3 pp0 = p - p0;

		glm::vec3 pp0xp2p0 = glm::cross(pp0, p2p0);
		float b = glm::dot(pp0xp2p0, n);
		if (b < 0.0f) 
			return false;

		glm::vec3 pp0xp1p0 = glm::cross(pp0, p1p0);
		float c = glm::dot(pp0xp1p0, -n);
		if (c < 0.0f) 
			return false;

		float denom = glm::dot(n, n);
		b /= denom;
		c /= denom;
		float a = 1.0f - b - c;
		if (a > 1.0f || a < 0.0f)
			return false;

		weights[0] = a;
		weights[1] = b;
		weights[2] = c;

		if (t < tMinDist)
			tMinDist = t;
		return true;
#else
		struct Vec3f
		{
			Vec3f(const glm::vec3& vIn)
			{
				v = vIn;
			}

			Vec3f operator - (const Vec3f& rhs) const
			{
				return v - rhs.v;
			}

			Vec3f crossProduct(const Vec3f& rhs) const
			{
				return glm::cross(v, rhs.v);
			}

			float dotProduct(const Vec3f& rhs) const
			{
				return glm::dot(v, rhs.v);
			}

			Vec3f operator * (float rhs) const
			{
				return v * rhs;
			}

			Vec3f operator + (const Vec3f& rhs) const
			{
				return v + rhs.v;
			}

			glm::vec3 v;
		};

		Vec3f v0 = p0;
		Vec3f v1 = p1;
		Vec3f v2 = p2;
		Vec3f orig = ray.Origin;
		Vec3f dir = ray.Direction;
		float t;
		float u, v;
		const float kEpsilon = RAY_PLANE_TOL;

		// compute plane's normal
		Vec3f v0v1 = v1 - v0;
		Vec3f v0v2 = v2 - v0;
		// no need to normalize
		Vec3f N = v0v1.crossProduct(v0v2); // N 
		float denom = N.dotProduct(N);

		// Step 1: finding P

		// check if ray and plane are parallel ?
		float NdotRayDirection = N.dotProduct(dir);
		if (fabs(NdotRayDirection) < kEpsilon) // almost 0 
			return false; // they are parallel so they don't intersect ! 

		// compute d parameter using equation 2
		float d = N.dotProduct(v0);

		// compute t (equation 3)
		t = (N.dotProduct(orig) + d) / NdotRayDirection;
		// check if the triangle is in behind the ray
		if (t < 0) return false; // the triangle is behind 

		// compute the intersection point using equation 1
		Vec3f P = orig + (dir * t);

		// Step 2: inside-outside test
		Vec3f C = glm::vec3(0.0f); // vector perpendicular to triangle's plane 

		// edge 0
		Vec3f edge0 = v1 - v0;
		Vec3f vp0 = P - v0;
		C = edge0.crossProduct(vp0);
		if (N.dotProduct(C) < 0) return false; // P is on the right side 

		// edge 1
		Vec3f edge1 = v2 - v1;
		Vec3f vp1 = P - v1;
		C = edge1.crossProduct(vp1);
		if ((u = N.dotProduct(C)) < 0)  return false; // P is on the right side 

		// edge 2
		Vec3f edge2 = v0 - v2;
		Vec3f vp2 = P - v2;
		C = edge2.crossProduct(vp2);
		if ((v = N.dotProduct(C)) < 0) return false; // P is on the right side; 

		u /= denom;
		v /= denom;

		if (t < tMinDist)
			tMinDist = t;

		weights[0] = u;
		weights[1] = v;
		weights[2] = 1.0f - u - v;

		return true; // this ray hits the triangle 
#endif
	}
}