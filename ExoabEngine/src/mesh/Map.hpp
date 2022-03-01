#pragma once
#include "../core/pipeline/Pipeline.hpp"
#include <memory/Buffer2.hpp>
#include <shaders/ShaderBinding.hpp>
#include <vector>
#include <glm/glm.hpp>

/*
    This file creates the map vertices
    and applies the height map. GPU buffers,
    pipelines, and shader sets are managed by
    the application.
*/

struct MapVertex
{
    glm::vec4 inPosition;
    glm::vec4 inNormal;
    glm::uvec4 inTextureIDs;
    glm::uvec4 inTextureWeights;
    glm::vec2 inTexCoords;
    int padding[2];
};

struct Map
{
    uint32_t m_width, m_height;
    std::vector<MapVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

Map Map_Create(int width, int xresolution, int height, int yresolution);
//TODO: void Map_ApplyHeightMap(Map* map, ...);
