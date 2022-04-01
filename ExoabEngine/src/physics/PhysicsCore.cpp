#include "PhysicsCore.hpp"

namespace Ph {

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


}