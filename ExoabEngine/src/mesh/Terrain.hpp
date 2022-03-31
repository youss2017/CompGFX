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
struct TerrainSubmesh {
    uint32_t mFirstVertex;
    uint32_t mFirstIndex;
    uint32_t mIndicesCount;
    glm::vec3 mBoundingBoxMin;
    glm::vec3 mBoundingBoxMax;
};

class Terrain {

public:
    // [WARNING] Only splits with even numbers, if its odds it will be rounded to even,
    // splitX creates horizontal blocks with size (width / splitX) same for splitY
    Terrain(uint32_t width, uint32_t height, uint32_t splitX, uint32_t splitY);
    ~Terrain();

    Terrain(const Terrain& other) = delete;
    Terrain(const Terrain&& other) = delete;

    void ApplyHeightmap(int heightMapWidth, int heightMapHeight, float minHeight, float maxHeight, uint8_t* heightmap);
    void SetTransform(const glm::mat4& transform) { mModelTransform = transform; }
    glm::mat4 GetTransform() { return mModelTransform; }
   
    IBuffer2 GetVerticesBuffer() { return mVerticesBuffer; }
    IBuffer2 GetIndicesBuffer() { return mIndicesBuffer; }

    inline uint32_t GetSubmeshCount() { return mSubmeshes.size(); }
    inline TerrainSubmesh& GetSubmesh(uint32_t i) { return mSubmeshes[i]; }
    //inline uint32_t GetIndicesCount() { return mIndices.size(); }

private:
    void CalculateTangentBitangent();

private:
    uint32_t mResolution;
    std::vector<TerrainVertex> mVertices;
    std::vector<uint32_t> mIndices;
    std::vector<TerrainSubmesh> mSubmeshes;
    IBuffer2 mVerticesBuffer;
    IBuffer2 mIndicesBuffer;
    glm::mat4 mModelTransform;
};
