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
struct TerrainInfo
{
    uint32_t m_width, m_height;
    std::vector<TerrainVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    uint32_t m_totalVerticesCount = 0;
    uint32_t m_totalIndicesCount = 0;
};

struct Terrain {
    IBuffer2 mVertices;
    IBuffer2 mIndices;
    uint32_t mIndicesCount;
    glm::mat4 mModelTransform;
};

TerrainInfo Terrain_Create(int width, int height);
void Terrain_ApplyHeightMap(TerrainInfo* terrain, int width, int height, float minHeight, float maxHeight, uint8_t* heightmap);
