#pragma once
#include "../core/pipeline/Pipeline.hpp"
#include <memory/Buffer2.hpp>
#include <shaders/ShaderConnector.hpp>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/cexport.h>
#include "../physics/PhysicsCore.hpp"

/*
    This file creates the map vertices
    and applies the height map. GPU buffers,
    pipelines, and shader sets are managed by
    the application.
*/

struct TerrainVertex
{
    Ph::hvec3 inPosition16;
    Ph::hvec3 inNormal16;
    Ph::hvec3 inTangent16;
    glm::u8vec3 inTextureIDs;
    Ph::hvec3 inTextureWeights;
    Ph::hvec2 inTexCoords;
};

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
    glm::mat4 GetToCenterTransform();
    // scales the terrain to a 1x1x1 cube
    glm::mat4 GetSquareTransform(float maxHeight);
   
    IBuffer2 GetVerticesBuffer() { return mVerticesBuffer; }
    IBuffer2 GetIndicesBuffer() { return mIndicesBuffer; }

    inline uint32_t GetSubmeshCount() { return mSubmeshes.size(); }
    inline TerrainSubmesh& GetSubmesh(uint32_t i) { return mSubmeshes[i]; }

    static std::vector<std::pair<std::string, std::string>> GetFilters(std::vector<std::string>& extensionIDs);
    void Export(const std::string& assimpID, const std::string& path, size_t* pOutSize = nullptr, char** pOutBuffer = nullptr);
    void Save(const std::string& name);

private:
    void CalculateTangentBitangent();
    void SmoothNormals();

private:
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mSplitX;
    uint32_t mSplitY;
    std::vector<TerrainVertex> mVertices;
    std::vector<uint32_t> mIndices;
    std::vector<TerrainSubmesh> mSubmeshes;
    IBuffer2 mVerticesBuffer;
    IBuffer2 mIndicesBuffer;
    glm::mat4 mModelTransform;
};
