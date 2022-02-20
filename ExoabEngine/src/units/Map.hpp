#pragma once
#include "../core/pipeline/Pipeline.hpp"
#include <memory/Buffer2.hpp>
#include <vector>

struct TerrainVertex
{
    glm::vec3 inPosition;
    glm::vec3 inNormal;
    glm::uvec3 inTextureIDs;
    glm::vec3 inTextureWeights;
    glm::vec2 inTexCoords;
};

struct Terrain
{
    GraphicsContext m_context;
    int m_width, m_height;
    std::vector<TerrainVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    IGPUTexture2D m_base_texture;
    IBuffer2 m_vertices_buffer;
    IBuffer2 m_indices_buffer;
    PipelineVertexInputDescription m_input_description;
    // TODO: Put Texture Map, Normal Map, Specular Map here
};

typedef Terrain* ITerrain;

// integers will round up to the nearest even number
// resolution refers to the amount of points between each vertices
#if 0
ITerrain Terrain_Create(IPipelineLayout layout, int width, int xresolution, int height, int yresolution, IGPUTextureSampler sampler, IGPUTexture2D basetexture, int setID, int bindingID);
void Terrain_Destroy(ITerrain terrain);
#endif