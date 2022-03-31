#pragma once
#include <glm/glm.hpp>
#include <glm/detail/type_half.hpp>

namespace Ph {

	using namespace glm;

	typedef glm::i16vec4 hvec4;
	typedef glm::i16vec3 hvec3;
	typedef glm::i16vec2 hvec2;
	typedef float float16;

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

	struct BoundingBox {
		vec3 mBoxMin;
		vec3 mBoxMax;
	};

	// Stride to next position
	BoundingBox CalculateBoundingBox(void* pVertices, uint32_t* pIndices, uint32_t indicesCount, uint32_t verticesStride);
	// Same as the original expect were working with hvec3 instead.
	BoundingBox CalculateBoundingBox16(void* pVertices, uint32_t* pIndices, uint32_t indicesCount, uint32_t verticesStride);

}
