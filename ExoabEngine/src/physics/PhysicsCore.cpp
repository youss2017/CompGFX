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
        for(uint32_t i = 0; i < indicesCount; i++) {
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


}