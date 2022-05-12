#pragma once
#include <glm/glm.hpp>
#include <glm/detail/type_half.hpp>
#include <vector>
#include <glm/gtc/quaternion.hpp>

namespace Ph {

	using namespace glm;

	typedef glm::i16vec4 hvec4;
	typedef glm::i16vec3 hvec3;
	typedef glm::i16vec2 hvec2;
	typedef float float16;

	struct Ray {
		glm::vec3 mOrigin;
		glm::vec3 mDirection;
	};

	inline hvec4 HalfVec4(const glm::vec4& v) {
		return hvec4(glm::detail::toFloat16(v.x), glm::detail::toFloat16(v.y), glm::detail::toFloat16(v.z), glm::detail::toFloat16(v.w));
	}

	inline hvec3 HalfVec3(const glm::vec3& v) {
		return hvec3(glm::detail::toFloat16(v.x), glm::detail::toFloat16(v.y), glm::detail::toFloat16(v.z));
	}

	inline hvec2 HalfVec2(const glm::vec2& v) {
		return hvec2(glm::detail::toFloat16(v.x), glm::detail::toFloat16(v.y));
	}

	inline glm::vec3 FullVec4(const hvec4& v) {
		return glm::vec4(glm::detail::toFloat32(v.x), glm::detail::toFloat32(v.y), glm::detail::toFloat32(v.z), glm::detail::toFloat32(v.w));
	}

	inline glm::vec3 FullVec3(const hvec3& v) {
		return glm::vec3(glm::detail::toFloat32(v.x), glm::detail::toFloat32(v.y), glm::detail::toFloat32(v.z));
	}

	inline glm::vec2 FullVec2(const hvec2& v) {
		return glm::vec2(glm::detail::toFloat32(v.x), glm::detail::toFloat32(v.y));
	}

	inline float16 HalfFloat(float v) {
		return glm::detail::toFloat16(v);
	}

	inline float FullFloat(float16 v) {
		return glm::detail::toFloat32(v);
	}

	inline i8vec3 Normal32To8(const vec3& normal) {
		return i8vec3(normal * vec3(127.0));
	}

	inline vec3 Normal8To32(const i8vec3& normal) {
		return vec3(normal) / 127.0f;
	}

	struct BoundingBox {
		vec3 mBoxMin;
		vec3 mBoxMax;
	};

	struct BoundingSphere {
		vec3 mCenter;
		float mRadius;
	};

	std::vector<float16> VFloat32ToFloat16(const std::vector<float>& source);
	std::vector<float16> VFloat32ToFloat16(const float* source, uint32_t count);
	std::vector<float16> VFloat64ToFloat16(const std::vector<double>& source);
	std::vector<float16> VFloat64ToFloat16(const double* source, uint32_t count);

	std::vector<float> VFloat16ToFloat32(const std::vector<float16>& source);
	std::vector<float> VFloat16ToFloat32(const float16* source, uint32_t count);
	std::vector<double> VFloat16ToFloat64(const std::vector<float16>& source);
	std::vector<double> VFloat16ToFloat64(const float16* source, uint32_t count);

	// Stride to next position
	BoundingBox CalculateBoundingBox(void* pVertices, uint32_t* pIndices, uint32_t indicesCount, uint32_t verticesStride);
	// Same as the original expect were working with hvec3 instead.
	BoundingBox CalculateBoundingBox16(void* pVertices, uint32_t* pIndices, uint32_t indicesCount, uint32_t verticesStride);
	BoundingSphere CalculateBoundingSphere(void* pVertices, uint32_t* pIndices, uint32_t indicesCount, uint32_t verticesStride);

	vec3 GenerateRayFromScreenCoordinates(const mat4& proj, const mat4& view, const vec2& xy, const vec2& windowSize);

	quat ExtractRotation(const mat4& matrix);
	vec3 ExtractPosition(const mat4& matrix);
	vec3 ExtractScale(const mat4& matrix);
	void DecomposeMatrix(mat4& matrix, vec3& localPosition, quat& localRotation, vec3& localScale);
	mat4 QuatToMatrix(quat& q);

}
