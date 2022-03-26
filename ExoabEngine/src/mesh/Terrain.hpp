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
    glm::vec4 inPosition;
    glm::vec4 inNormal;
    glm::ivec4 inTextureIDs;
    glm::vec4 inTextureWeights;
    glm::vec2 inTexCoords;
};

// TODO: Split map into smaller squares so we can do frustrum culling on the map.
struct Terrain
{
    uint32_t m_width, m_height;
    std::vector<TerrainVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    uint32_t m_totalVerticesCount = 0;
    uint32_t m_totalIndicesCount = 0;
};

Terrain Terrain_Create(int width, int xresolution, int height, int yresolution, int divide_count);
//TODO: void Map_ApplyHeightMap(Map* map, ...);
