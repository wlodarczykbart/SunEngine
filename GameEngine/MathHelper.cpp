#include "MathHelper.h"

namespace SunEngine
{
	const glm::vec4 Vec4::Zero = glm::vec4(0.0f);
	const glm::vec4 Vec4::Point = glm::vec4(0.0f, 0.0f, 0.0, 1.0f);
	const glm::vec4 Vec4::One = glm::vec4(1.0f);
	const glm::vec4 Vec4::Half = glm::vec4(0.5f);
	const glm::vec4 Vec4::Right = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
	const glm::vec4 Vec4::Left = glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f);
	const glm::vec4 Vec4::Up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	const glm::vec4 Vec4::Down = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
	const glm::vec4 Vec4::Forward = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
	const glm::vec4 Vec4::Back = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
	const glm::vec4 Vec4::Min = glm::vec4(-FLT_MAX);
	const glm::vec4 Vec4::Max = glm::vec4(FLT_MAX);

	const glm::vec3 Vec3::Zero = glm::vec3(0.0f);
	const glm::vec3 Vec3::One = glm::vec3(1.0f);
	const glm::vec3 Vec3::Half = glm::vec3(0.5f);
	const glm::vec3 Vec3::Right = glm::vec3(1.0f, 0.0f, 0.0f);
	const glm::vec3 Vec3::Left = glm::vec3(-1.0f, 0.0f, 0.0f);
	const glm::vec3 Vec3::Up = glm::vec3(0.0f, 1.0f, 0.0f);
	const glm::vec3 Vec3::Down = glm::vec3(0.0f, -1.0f, 0.0f);
	const glm::vec3 Vec3::Forward = glm::vec3(0.0f, 0.0f, 1.0f);
	const glm::vec3 Vec3::Back = glm::vec3(0.0f, 0.0f, -1.0f);
	const glm::vec3 Vec3::Min = glm::vec3(-FLT_MAX);
	const glm::vec3 Vec3::Max = glm::vec3(FLT_MAX);

	const glm::mat4 Mat4::Identity = glm::mat4(1.0f);
	const glm::mat4 Mat4::Zero = glm::mat4(0.0f);

	const glm::quat Quat::Identity = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}