#pragma once

#include "Types.h"
#include "glm/glm.hpp"

namespace SunEngine
{
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

		glm::vec3 Min;
		glm::vec3 Max;
	};
}