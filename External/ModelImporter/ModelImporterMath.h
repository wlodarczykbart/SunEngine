#pragma once

#include <math.h>

namespace ModelImporter
{
	const float PI = 3.14159f;

	inline bool ApproxEqual(float lhs, float rhs)
	{
		return fabsf(lhs - rhs) < 0.001f;
	}

	inline float Deg2Rad(float deg)
	{
		return deg * (PI / 180.0f);
	}

	inline float Rad2Deg(float rad)
	{
		return rad * (180.0f / PI);
	}

	struct Mat4
	{
		Mat4()
		{
			r0[0] = 1.0f;
			r0[1] = 0.0f;
			r0[2] = 0.0f;
			r0[3] = 0.0f;
					
			r1[0] = 0.0f;
			r1[1] = 1.0f;
			r1[2] = 0.0f;
			r1[3] = 0.0f;
					
			r2[0] = 0.0f;
			r2[1] = 0.0f;
			r2[2] = 1.0f;
			r2[3] = 0.0f;
					
			r3[0] = 0.0f;
			r3[1] = 0.0f;
			r3[2] = 0.0f;
			r3[3] = 1.0f;
		}

		inline float* operator[](int idx)
		{
			if (idx == 0) return r0;
			if (idx == 1) return r1;
			if (idx == 2) return r2;
			if (idx == 3) return r3;
			return NULL;
		}

		inline Mat4 operator * (const Mat4& rhs) const
		{
			Mat4 mtx;
			mtx.r0[0] = r0[0] * rhs.r0[0] + r0[1] * rhs.r1[0] + r0[2] * rhs.r2[0] + r0[3] * rhs.r3[0];
			mtx.r0[1] = r0[0] * rhs.r0[1] + r0[1] * rhs.r1[1] + r0[2] * rhs.r2[1] + r0[3] * rhs.r3[1];
			mtx.r0[2] = r0[0] * rhs.r0[2] + r0[1] * rhs.r1[2] + r0[2] * rhs.r2[2] + r0[3] * rhs.r3[2];
			mtx.r0[3] = r0[0] * rhs.r0[3] + r0[1] * rhs.r1[3] + r0[2] * rhs.r2[3] + r0[3] * rhs.r3[3];

			mtx.r1[0] = r1[0] * rhs.r0[0] + r1[1] * rhs.r1[0] + r1[2] * rhs.r2[0] + r1[3] * rhs.r3[0];
			mtx.r1[1] = r1[0] * rhs.r0[1] + r1[1] * rhs.r1[1] + r1[2] * rhs.r2[1] + r1[3] * rhs.r3[1];
			mtx.r1[2] = r1[0] * rhs.r0[2] + r1[1] * rhs.r1[2] + r1[2] * rhs.r2[2] + r1[3] * rhs.r3[2];
			mtx.r1[3] = r1[0] * rhs.r0[3] + r1[1] * rhs.r1[3] + r1[2] * rhs.r2[3] + r1[3] * rhs.r3[3];

			mtx.r2[0] = r2[0] * rhs.r0[0] + r2[1] * rhs.r1[0] + r2[2] * rhs.r2[0] + r2[3] * rhs.r3[0];
			mtx.r2[1] = r2[0] * rhs.r0[1] + r2[1] * rhs.r1[1] + r2[2] * rhs.r2[1] + r2[3] * rhs.r3[1];
			mtx.r2[2] = r2[0] * rhs.r0[2] + r2[1] * rhs.r1[2] + r2[2] * rhs.r2[2] + r2[3] * rhs.r3[2];
			mtx.r2[3] = r2[0] * rhs.r0[3] + r2[1] * rhs.r1[3] + r2[2] * rhs.r2[3] + r2[3] * rhs.r3[3];

			mtx.r3[0] = r3[0] * rhs.r0[0] + r3[1] * rhs.r1[0] + r3[2] * rhs.r2[0] + r3[3] * rhs.r3[0];
			mtx.r3[1] = r3[0] * rhs.r0[1] + r3[1] * rhs.r1[1] + r3[2] * rhs.r2[1] + r3[3] * rhs.r3[1];
			mtx.r3[2] = r3[0] * rhs.r0[2] + r3[1] * rhs.r1[2] + r3[2] * rhs.r2[2] + r3[3] * rhs.r3[2];
			mtx.r3[3] = r3[0] * rhs.r0[3] + r3[1] * rhs.r1[3] + r3[2] * rhs.r2[3] + r3[3] * rhs.r3[3];

			return mtx;
		}			

		inline Mat4 operator + (const Mat4& rhs) const
		{
			Mat4 mtx;
			mtx.r0[0] = r0[0] + rhs.r0[0];
			mtx.r0[1] = r0[1] + rhs.r0[1];
			mtx.r0[2] = r0[2] + rhs.r0[2];
			mtx.r0[3] = r0[3] + rhs.r0[3];
							  
			mtx.r1[0] = r1[0] + rhs.r1[0];
			mtx.r1[1] = r1[1] + rhs.r1[1];
			mtx.r1[2] = r1[2] + rhs.r1[2];
			mtx.r1[3] = r1[3] + rhs.r1[3];
							  
			mtx.r2[0] = r2[0] + rhs.r2[0];
			mtx.r2[1] = r2[1] + rhs.r2[1];
			mtx.r2[2] = r2[2] + rhs.r2[2];
			mtx.r2[3] = r2[3] + rhs.r2[3];
							  
			mtx.r3[0] = r3[0] + rhs.r3[0];
			mtx.r3[1] = r3[1] + rhs.r3[1];
			mtx.r3[2] = r3[2] + rhs.r3[2];
			mtx.r3[3] = r3[3] + rhs.r3[3];

			return mtx;
		}	

		inline Mat4 operator * (float rhs) const
		{
			Mat4 mtx;
			mtx.r0[0] = r0[0] * rhs;
			mtx.r0[1] = r0[1] * rhs;
			mtx.r0[2] = r0[2] * rhs;
			mtx.r0[3] = r0[3] * rhs;
							  
			mtx.r1[0] = r1[0] * rhs;
			mtx.r1[1] = r1[1] * rhs;
			mtx.r1[2] = r1[2] * rhs;
			mtx.r1[3] = r1[3] * rhs;
							  
			mtx.r2[0] = r2[0] * rhs;
			mtx.r2[1] = r2[1] * rhs;
			mtx.r2[2] = r2[2] * rhs;
			mtx.r2[3] = r2[3] * rhs;
							  
			mtx.r3[0] = r3[0] * rhs;
			mtx.r3[1] = r3[1] * rhs;
			mtx.r3[2] = r3[2] * rhs;
			mtx.r3[3] = r3[3] * rhs;

			return mtx;
		}	

		inline bool operator == (const Mat4& rhs) const
		{
			if(!ApproxEqual(r0[0], rhs.r0[0])) return false;
			if(!ApproxEqual(r0[1], rhs.r0[1])) return false;
			if(!ApproxEqual(r0[2], rhs.r0[2])) return false;
			if(!ApproxEqual(r0[3], rhs.r0[3])) return false;

			if(!ApproxEqual(r1[0], rhs.r1[0])) return false;
			if(!ApproxEqual(r1[1], rhs.r1[1])) return false;
			if(!ApproxEqual(r1[2], rhs.r1[2])) return false;
			if(!ApproxEqual(r1[3], rhs.r1[3])) return false;

			if(!ApproxEqual(r2[0], rhs.r2[0])) return false;
			if(!ApproxEqual(r2[1], rhs.r2[1])) return false;
			if(!ApproxEqual(r2[2], rhs.r2[2])) return false;
			if(!ApproxEqual(r2[3], rhs.r2[3])) return false;

			if(!ApproxEqual(r3[0], rhs.r3[0])) return false;
			if(!ApproxEqual(r3[1], rhs.r3[1])) return false;
			if(!ApproxEqual(r3[2], rhs.r3[2])) return false;
			if(!ApproxEqual(r3[3], rhs.r3[3])) return false;

			return true;
		}

		inline bool operator != (const Mat4& rhs) const
		{
			return !(operator == (rhs));
		}

		inline Mat4 GetTranspose() const
		{
			Mat4 mtx;

			mtx.r0[0] = r0[0];
			mtx.r0[1] = r1[0];
			mtx.r0[2] = r2[0];
			mtx.r0[3] = r3[0];

			mtx.r1[0] = r0[1];
			mtx.r1[1] = r1[1];
			mtx.r1[2] = r2[1];
			mtx.r1[3] = r3[1];

			mtx.r2[0] = r0[2];
			mtx.r2[1] = r1[2];
			mtx.r2[2] = r2[2];
			mtx.r2[3] = r3[2];

			mtx.r3[0] = r0[3];
			mtx.r3[1] = r1[3];
			mtx.r3[2] = r2[3];
			mtx.r3[3] = r3[3];
			
			return mtx;
		}

		inline float GetDet3x3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22) const
		{
			return m20 * (m01 * m12 - m02 * m11) + m21 * (m02 * m10 - m00 * m12) + m22 * (m00 * m11 - m01 * m10);
		}

		inline Mat4 GetInverse() const
		{
			float detm00 = GetDet3x3(r1[1], r1[2], r1[3], r2[1], r2[2], r2[3], r3[1], r3[2], r3[3]);
			float detm01 = GetDet3x3(r1[0], r1[2], r1[3], r2[0], r2[2], r2[3], r3[0], r3[2], r3[3]);
			float detm02 = GetDet3x3(r1[0], r1[1], r1[3], r2[0], r2[1], r2[3], r3[0], r3[1], r3[3]);
			float detm03 = GetDet3x3(r1[0], r1[1], r1[2], r2[0], r2[1], r2[2], r3[0], r3[1], r3[2]);

			float det = detm00 * r0[0] - detm01 * r0[1] + detm02 * r0[2] - detm03 * r0[3];
			if (det != 0.0f)
			{
				det = 1.0f / det;

				Mat4 mtx;

				mtx.r0[0] = detm00 * det;
				mtx.r1[0] = detm01 * -det;
				mtx.r2[0] = detm02 * det;
				mtx.r3[0] = detm03 * -det;

				mtx.r0[1] = GetDet3x3(r0[1], r0[2], r0[3], r2[1], r2[2], r2[3], r3[1], r3[2], r3[3]) * -det;
				mtx.r1[1] = GetDet3x3(r0[0], r0[2], r0[3], r2[0], r2[2], r2[3], r3[0], r3[2], r3[3]) * det;
				mtx.r2[1] = GetDet3x3(r0[0], r0[1], r0[3], r2[0], r2[1], r2[3], r3[0], r3[1], r3[3]) * -det;
				mtx.r3[1] = GetDet3x3(r0[0], r0[1], r0[2], r2[0], r2[1], r2[2], r3[0], r3[1], r3[2]) * det;

				mtx.r0[2] = GetDet3x3(r0[1], r0[2], r0[3], r1[1], r1[2], r1[3], r3[1], r3[2], r3[3]) * det;
				mtx.r1[2] = GetDet3x3(r0[0], r0[2], r0[3], r1[0], r1[2], r1[3], r3[0], r3[2], r3[3]) * -det;
				mtx.r2[2] = GetDet3x3(r0[0], r0[1], r0[3], r1[0], r1[1], r1[3], r3[0], r3[1], r3[3]) * det;
				mtx.r3[2] = GetDet3x3(r0[0], r0[1], r0[2], r1[0], r1[1], r1[2], r3[0], r3[1], r3[2]) * -det;
					   
				mtx.r0[3] = GetDet3x3(r0[1], r0[2], r0[3], r1[1], r1[2], r1[3], r2[1], r2[2], r2[3]) * -det;
				mtx.r1[3] = GetDet3x3(r0[0], r0[2], r0[3], r1[0], r1[2], r1[3], r2[0], r2[2], r2[3]) * det;
				mtx.r2[3] = GetDet3x3(r0[0], r0[1], r0[3], r1[0], r1[1], r1[3], r2[0], r2[1], r2[3]) * -det;
				mtx.r3[3] = GetDet3x3(r0[0], r0[1], r0[2], r1[0], r1[1], r1[2], r2[0], r2[1], r2[2]) * det;

				return mtx;
			}
			else
			{
				//non invertable
				return *this;
			}
		}

		void RotateX(float angleRad)
		{
			r0[0] = 1.0f;
			r0[1] = 0.0f;
			r0[2] = 0.0f;
			r0[3] = 0.0f;

			r1[0] = 0.0f;
			r1[1] = cosf(angleRad);
			r1[2] = sinf(angleRad);
			r1[3] = 0.0f;

			r2[0] = 0.0f;
			r2[1] = -sinf(angleRad);
			r2[2] = cosf(angleRad);
			r2[3] = 0.0f;

			r3[0] = 0.0f;
			r3[1] = 0.0f;
			r3[2] = 0.0f;
			r3[3] = 1.0f;
		}

		void RotateY(float angleRad)
		{
			r0[0] = cosf(angleRad);
			r0[1] = 0.0f;
			r0[2] = -sinf(angleRad);
			r0[3] = 0.0f;

			r1[0] = 0.0f;
			r1[1] = 1.0f;
			r1[2] = 0.0f;
			r1[3] = 0.0f;

			r2[0] = sinf(angleRad);
			r2[1] = 0.0f;
			r2[2] = cosf(angleRad);
			r2[3] = 0.0f;

			r3[0] = 0.0f;
			r3[1] = 0.0f;
			r3[2] = 0.0f;
			r3[3] = 1.0f;
		}

		void RotateZ(float angleRad)
		{
			r0[0] = cosf(angleRad);
			r0[1] = sinf(angleRad);
			r0[2] = 0.0f;
			r0[3] = 0.0f;

			r1[0] = -sinf(angleRad);
			r1[1] = cosf(angleRad);
			r1[2] = 0.0f;
			r1[3] = 0.0f;

			r2[0] = 0.0f;
			r2[1] = 0.0f;
			r2[2] = 1.0f;
			r2[3] = 0.0f;

			r3[0] = 0.0f;
			r3[1] = 0.0f;
			r3[2] = 0.0f;
			r3[3] = 1.0f;
		}

		void RotateXY(float angleXRad, float angleYRad)
		{
			Mat4 mx, my;
			mx.RotateX(angleXRad);
			my.RotateY(angleYRad);

			*this = mx * my;
		}

		void RotateXYZ(float angleXRad, float angleYRad, float angleZRad)
		{
			Mat4 mx, my, mz;
			mx.RotateX(angleXRad);
			my.RotateY(angleYRad);
			mz.RotateZ(angleZRad);

			*this = mx * my * mz;
		}

		void SetTranslation(float tx, float ty, float tz)
		{
			r3[0] = tx;
			r3[1] = ty;
			r3[2] = tz;
			r3[3] = 1.0f;
		}

		void SetScale(float sx, float sy, float sz)
		{
			r0[0] = sx;
			r1[1] = sy;
			r2[2] = sz;
		}

		float r0[4];
		float r1[4];
		float r2[4];
		float r3[4];
	};

	struct Vec2
	{
	public:
		Vec2()
		{
			SetZero();
		}

		Vec2(float _x, float _y) 
		{
			Set(_x, _y);
		}

		inline void SetOne()
		{
			Set(1.0f);
		}

		inline void SetZero()
		{
			Set(0.0f);
		}

		inline void Set(float xy)
		{
			x = y = xy;
		}

		inline void Set(float _x, float _y)
		{
			x = _x;
			y = _y;
		}

		inline Vec2 operator + (const Vec2& rhs) const
		{
			return Vec2(x + rhs.x, y + rhs.y);
		}

		inline Vec2 operator - (const Vec2& rhs) const
		{
			return Vec2(x - rhs.x, y - rhs.y);
		}

		inline Vec2 operator * (float rhs) const
		{
			return Vec2(x * rhs, y * rhs);
		}

		inline void operator += (const Vec2& rhs)
		{
			x += rhs.x;
			y += rhs.y;
		}

		inline void operator -= (const Vec2& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
		}

		inline void operator *= (float rhs)
		{
			x *= rhs;
			y *= rhs;
		}	

		inline bool operator == (const Vec2& rhs) const
		{
			return ApproxEqual(x, rhs.x) && ApproxEqual(y, rhs.y);
		}

		float x;
		float y;
	};

	struct Vec3
	{
	public:
		Vec3()
		{
			SetZero();
		}

		Vec3(float _x, float _y, float _z) 
		{
			Set(_x, _y, _z);
		}

		inline float Dot(const Vec3& rhs) const
		{
			return x * rhs.x + y * rhs.y + z * rhs.z;
		}

		inline float GetLengthSquared() const
		{
			return x * x + y * y + z * z;
		}

		inline float GetLength()
		{
			float squared = GetLengthSquared();
			return squared != 0.0 ? sqrtf(squared) : 0.0f;
		}

		inline float GetOneOverLength() const
		{
			float squared = GetLengthSquared();
			return squared != 0.0f ? 1.0f / sqrtf(squared) : 0.0f;
		}

		inline void Norm()
		{
			float length = GetOneOverLength();
			x *= length;
			y *= length;
			z *= length;
		}

		inline Vec3 Cross(const Vec3& rhs) const
		{
			return Vec3(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
		}

		inline void Orthogonalize(const Vec3& perpVector)
		{
			Vec3 parallelProjection = perpVector * Dot(perpVector);
			x -= parallelProjection.x;
			y -= parallelProjection.y;
			z -= parallelProjection.z;
		}

		inline void SetOne()
		{
			Set(1.0f);
		}

		inline void SetZero()
		{
			Set(0.0f);
		}

		inline void Set(float xyz)
		{
			x = y = z = xyz;
		}

		inline void Set(float _x, float _y, float _z)
		{
			x = _x;
			y = _y;
			z = _z;
		}

		inline Vec3 operator + (const Vec3& rhs) const
		{
			return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
		}

		inline Vec3 operator - (const Vec3& rhs) const
		{
			return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
		}

		inline Vec3 operator * (float rhs) const
		{
			return Vec3(x * rhs, y * rhs, z * rhs);
		}

		inline void operator += (const Vec3& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
		}

		inline void operator -= (const Vec3& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			z -= rhs.z;
		}

		inline void operator *= (float rhs)
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
		}	  

		inline bool operator == (const Vec3& rhs) const
		{
			return ApproxEqual(x, rhs.x) && ApproxEqual(y, rhs.y) && ApproxEqual(z, rhs.z);
		}

		inline void AffineTransform(const struct Mat4& mtx)
		{
			LinearTransform(mtx);
			x += mtx.r3[0];
			y += mtx.r3[1];
			z += mtx.r3[2];
		}

		inline void LinearTransform(const struct Mat4& mtx)
		{
			float tx = x * mtx.r0[0] + y * mtx.r1[0] + z * mtx.r2[0];
			float ty = x * mtx.r0[1] + y * mtx.r1[1] + z * mtx.r2[1];
			float tz = x * mtx.r0[2] + y * mtx.r1[2] + z * mtx.r2[2];
			Set(tx, ty, tz);
		}

		float x;
		float y;
		float z;


	};

	struct AABB
	{
		AABB()
		{
			Reset();
		}

		inline void Reset()
		{
			min.Set(+100000000.0f);
			max.Set(-100000000.0f);
		}

		inline void Update(const Vec3& data)
		{
			if (data.x < min.x)
				min.x = data.x;
			if (data.y < min.y)
				min.y = data.y;
			if (data.z < min.z)
				min.z = data.z;

			if (data.x > max.x)
				max.x = data.x;
			if (data.y > max.y)
				max.y = data.y;
			if (data.z > max.z)
				max.z = data.z;
		}

		inline void Update(const AABB& data)
		{
			Update(data.min);
			Update(data.max);
		}

		inline Vec3 GetCenter() const
		{
			return min + ((max - min) * 0.5f);
		}

		AABB GetTransformed(const Mat4& mtx) const
		{
			AABB res;
			
			Vec3 points[8];
			points[0] = min;
			points[1] = max;

			points[2].Set(min.x, max.y, min.z);
			points[4].Set(max.x, min.y, min.z);
			points[3].Set(max.x, max.y, min.z);

			points[5].Set(min.x, max.y, max.z);
			points[6].Set(max.x, min.y, max.z);
			points[7].Set(min.x, min.y, max.z);

			for (int i = 0; i < 8; i++)
			{
				points[i].AffineTransform(mtx);
				res.Update(points[i]);
			}

			return res;
		}

		Vec3 min;
		Vec3 max;
	};

}