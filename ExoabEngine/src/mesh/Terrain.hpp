#pragma once
#include "../core/pipeline/Pipeline.hpp"
#include <memory/Buffer2.hpp>
#include <shaders/ShaderConnector.hpp>
#include <vector>
#include <glm/glm.hpp>

/*
    This file creates the map vertices
    and applies the height map. GPU buffers,
    pipelines, and shader sets are managed by
    the application.
*/

#pragma pack(1)
struct TerrainVertex
{
    glm::vec3 inPosition;
    glm::vec3 inNormal;
    glm::vec3 inTangent;
    glm::vec3 inBiTangent;
    glm::ivec3 inTextureIDs;
    glm::vec3 inTextureWeights;
    glm::vec2 inTexCoords;
};

// TODO: Split map into smaller squares so we can do frustrum culling on the map.
class Terrain {

public:
    Terrain(uint32_t resolution, uint32_t splitEveryAmountOfVerts);
    ~Terrain();

    Terrain(const Terrain& other) = delete;
    Terrain(const Terrain&& other) = delete;

    void ApplyHeightmap(int heightMapWidth, int heightMapHeight, float minHeight, float maxHeight, uint8_t* heightmap);
    void SetTransform(const glm::mat4& transform) { mModelTransform = transform; }
    glm::mat4 GetTransform() { return mModelTransform; }
    uint32_t GetSubmeshCount() {
        return mVerticesIndicesOffset.size();
    }

    uint32_t GetVerticesOffset(uint32_t submeshIndex) {
        return mVerticesIndicesOffset[submeshIndex].first;
    }

    uint32_t GetIndicesOffset(uint32_t submeshIndex) {
        return mVerticesIndicesOffset[submeshIndex].second;
    }

    uint32_t GetVerticesCount(uint32_t submeshIndex) {
        return mVerticesIndicesCount[submeshIndex].first;
    }

    uint32_t GetIndicesCount(uint32_t submeshIndex) {
        return mVerticesIndicesCount[submeshIndex].second;
    }

    IBuffer2 GetVerticesBuffer() { return mVerticesBuffer; }
    IBuffer2 GetIndicesBuffer() { return mIndicesBuffer; }

private:
    void CalculateTangentBitangent();

private:
    uint32_t mResolution;
    std::vector<TerrainVertex> mVertices;
    std::vector<uint32_t> mIndices;
    IBuffer2 mVerticesBuffer;
    IBuffer2 mIndicesBuffer;
    std::vector<std::pair<uint32_t, uint32_t>> mVerticesIndicesOffset;
    std::vector<std::pair<uint32_t, uint32_t>> mVerticesIndicesCount;
    glm::mat4 mModelTransform;
};
