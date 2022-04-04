#pragma once
#include "../core/pipeline/Pipeline.hpp"
#include <memory/Buffer2.hpp>
#include <shaders/ShaderConnector.hpp>
#include <vector>
#include <glm/glm.hpp>
#include "../physics/PhysicsCore.hpp"

/*
    This file creates the map vertices
    and applies the height map. GPU buffers,
    pipelines, and shader sets are managed by
    the application.
*/

struct TerrainVertex
{
    glm::vec3 inPosition;
    Ph::hvec3 inNormal;
    Ph::hvec3 inTangent;
    Ph::hvec3 inBiTangent;
    glm::i16vec3 inTextureIDs;
    Ph::hvec3 inTextureWeights;
    Ph::hvec2 inTexCoords;
};

// TODO: Split map into smaller squares so we can do frustrum culling on the map.
struct TerrainSubmesh {
    uint32_t mFirstVertex;
    uint32_t mFirstIndex;
    uint32_t mIndicesCount;
    Ph::BoundingSphere mSphere;
};

struct TerrainHeightMap {
    float mContribution;
    float* mHeightMap;
};

class Terrain {

public:
    // [WARNING] Only splits with even numbers, if its odds it will be rounded to even,
    // splitX creates horizontal blocks with size (width / splitX) same for splitY
    Terrain(uint32_t width, uint32_t height, uint32_t splitX, uint32_t splitY);
    ~Terrain();

    Terrain(const Terrain& other) = delete;
    Terrain(const Terrain&& other) = delete;

    void ApplyHeightmap(int heightMapWidth, int heightMapHeight, float minHeight, float maxHeight, std::vector<TerrainHeightMap> heightMaps);
    void SetTransform(const glm::mat4& transform) { mModelTransform = transform;  }
    glm::mat4 GetTransform() { return mModelTransform; }
   
    IBuffer2 GetVerticesBuffer() { return mVerticesBuffer; }
    IBuffer2 GetIndicesBuffer() { return mIndicesBuffer; }

    inline uint32_t GetSubmeshCount() { return mSubmeshes.size(); }
    inline TerrainSubmesh& GetSubmesh(uint32_t i) { return mSubmeshes[i]; }

private:
    void CalculateTangentBitangent();

private:
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mResolutionX;
    uint32_t mResolutionY;
    std::vector<TerrainVertex> mVertices;
    std::vector<uint32_t> mIndices;
    std::vector<TerrainSubmesh> mSubmeshes;
    IBuffer2 mVerticesBuffer;
    IBuffer2 mIndicesBuffer;
    glm::mat4 mModelTransform;
};
