#include "PhysicsCore.hpp"

namespace Ph {
	std::vector<float16> VFloat32ToFloat16(const std::vector<float>& source)
	{
		return VFloat32ToFloat16(source.data(), source.size());
	}

	std::vector<float16> VFloat32ToFloat16(const float* source, uint32_t count)
	{
		std::vector<float16> conversion(count);
		for (uint32_t i = 0; i < count; i++) {
			conversion[i] = HalfFloat(source[i]);
		}
		return conversion;
	}

	std::vector<float16> VFloat64ToFloat16(const std::vector<double>& source)
	{
		return VFloat64ToFloat16(source.data(), source.size());
	}

	std::vector<float16> VFloat64ToFloat16(const double* source, uint32_t count)
	{
		std::vector<float16> conversion(count);
		for (uint32_t i = 0; i < count; i++) {
			conversion[i] = HalfFloat(float(source[i]));
		}
		return conversion;
	}

	std::vector<float> VFloat16ToFloat32(const std::vector<float16>& source)
	{
		return VFloat16ToFloat32(source.data(), source.size());
	}

	std::vector<float> VFloat16ToFloat32(const float16* source, uint32_t count)
	{
		std::vector<float> conversion(count);
		for (uint32_t i = 0; i < count; i++) {
			conversion[i] = FullFloat(source[i]);
		}
		return conversion;
	}

	std::vector<double> VFloat16ToFloat64(const std::vector<float16>& source)
	{
		return VFloat16ToFloat64(source.data(), source.size());
	}

	std::vector<double> VFloat16ToFloat64(const float16* source, uint32_t count)
	{
		std::vector<double> conversion(count);
		for (uint32_t i = 0; i < count; i++) {
			conversion[i] = double(FullFloat(source[i]));
		}
		return conversion;
	}

	BoundingBox CalculateBoundingBox(void* pVertices, uint32_t* pIndices, uint32_t indicesCount, uint32_t verticesStride) {
		char* pVertices8 = (char*)pVertices;
		float minx = INFINITY;
		float miny = INFINITY;
		float minz = INFINITY;
		float maxx = -INFINITY;
		float maxy = -INFINITY;
		float maxz = -INFINITY;
		for (uint32_t i = 0; i < indicesCount; i++) {
			uint32_t index = pIndices[i];
			vec3 position = *(vec3*)&pVertices8[index * verticesStride];
			minx = min(position.x, minx); maxx = max(position.x, maxx);
			miny = min(position.y, miny); maxy = max(position.y, maxy);
			minz = min(position.z, minz); maxz = max(position.z, maxz);
		}
		BoundingBox box;
		box.mBoxMin = vec3(minx, miny, minz);
		box.mBoxMax = vec3(maxx, maxy, maxz);
		return box;
	}

	BoundingBox CalculateBoundingBox16(void* pVertices, uint32_t* pIndices, uint32_t indicesCount, uint32_t verticesStride)
	{
		char* pVertices8 = (char*)pVertices;
		float minx = INFINITY;
		float miny = INFINITY;
		float minz = INFINITY;
		float maxx = -INFINITY;
		float maxy = -INFINITY;
		float maxz = -INFINITY;
		for (uint32_t i = 0; i < indicesCount; i++) {
			uint32_t index = pIndices[i];
			vec3 position = FullVec3(*(hvec3*)&pVertices8[index * verticesStride]);
			minx = min(position.x, minx); maxx = max(position.x, maxx);
			miny = min(position.y, miny); maxy = max(position.y, maxy);
			minz = min(position.z, minz); maxz = max(position.z, maxz);
		}
		BoundingBox box;
		box.mBoxMin = vec3(minx, miny, minz);
		box.mBoxMax = vec3(maxx, maxy, maxz);
		return box;
	}

	BoundingSphere CalculateBoundingSphere(void* pVertices, uint32_t* pIndices, uint32_t indicesCount, uint32_t verticesStride) {
		char* pVertices8 = (char*)pVertices;
		vec3 center = vec3(0.0);
		for (uint32_t i = 0; i < indicesCount; i++) {
			vec3 p = *(vec3*)&pVertices8[pIndices[i] * verticesStride];
			center += p;
		}
		center /= indicesCount;
		float radius = 0.0;
		for (uint32_t i = 0; i < indicesCount; i++) {
			vec3 p = *(vec3*)&pVertices8[pIndices[i] * verticesStride];
			radius = max(radius, abs(distance(center, p)));
		}
		return { center, radius };
	}

	vec3 GenerateRayFromScreenCoordinates(const mat4& proj, const mat4& view, const vec2& xy, const vec2& windowSize)
	{
		// convert window coords to [0,1] range
		vec2 coord = xy / windowSize;
		// convert from [0,1] range to [-1,1];
		coord = coord * 2.0f - 1.0f;
		// undo perspective projection and view projection
		mat4 invProjView = inverse(proj * view);
		vec4 coord4D = invProjView * vec4(coord, 1.0f, 1.0f);
		vec3 coord3D = normalize(vec3(coord4D) * coord4D.w);
		return coord3D;
	}

	quat ExtractRotation(const mat4& matrix)
	{
		vec3 forward;
		forward.x = matrix[2][0];
		forward.y = matrix[2][1];
		forward.z = matrix[2][2];

		vec3 upwards;
		upwards.x = matrix[1][0];
		upwards.y = matrix[1][1];
		upwards.z = matrix[1][2];

		return quatLookAt(forward, upwards);
	}

	vec3 ExtractPosition(const mat4& matrix)
	{
		vec3 position;
		position.x = matrix[3][0];
		position.y = matrix[3][1];
		position.z = matrix[3][2];
		return position;
	}

	vec3 ExtractScale(const mat4& matrix)
	{
		vec3 scale;
		scale.x = length(vec4(matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3]));
		scale.y = length(vec4(matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3]));
		scale.z = length(vec4(matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3]));
		return scale;
	}

	void DecomposeMatrix(mat4& matrix, vec3& localPosition, quat& localRotation, vec3& localScale) {
		localPosition = ExtractPosition(matrix);
		localRotation = ExtractRotation(matrix);
		localScale = ExtractScale(matrix);
	}

	// https://stackoverflow.com/questions/1556260/convert-quaternion-rotation-to-rotation-matrix
	mat4 QuatToMatrix(quat& q) {
		float qx = q.x;
		float qy = q.y;
		float qz = q.z;
		float qw = q.w;
		mat4 m = {
			1.0f - 2.0f * qy * qy - 2.0f * qz * qz, 2.0f * qx * qy - 2.0f * qz * qw, 2.0f * qx * qz + 2.0f * qy * qw, 0.0f,
			2.0f * qx * qy + 2.0f * qz * qw, 1.0f - 2.0f * qx * qx - 2.0f * qz * qz, 2.0f * qy * qz - 2.0f * qx * qw, 0.0f,
			2.0f * qx * qz - 2.0f * qy * qw, 2.0f * qy * qz + 2.0f * qx * qw, 1.0f - 2.0f * qx * qx - 2.0f * qy * qy, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		return transpose(m);
	}


}