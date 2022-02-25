#pragma once
#include "../core/pipeline/Pipeline.hpp"
#include <memory/Buffer2.hpp>
#include <shaders/ShaderBinding.hpp>
#include <vector>
#include <glm/glm.hpp>

struct TerrainVertex
{
    uint16_t inPosition[4];
    uint8_t inNormal[4];
    uint8_t inTextureIDs[4];
    uint8_t inTextureWeights[4];
    uint16_t inTexCoords[2];
};

struct Terrain
{
    GraphicsContext m_context;
    int m_width, m_height;
    std::vector<TerrainVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    IBuffer2 m_vertices_ssbo;
    IBuffer2 m_indices_buffer;
    ShaderSet m_Set0;
    ShaderSet m_Set1;
    VkDescriptorPool m_pool;
    VkPipelineLayout m_layout;
};

typedef Terrain* ITerrain;

ITerrain Terrain_Create(GraphicsContext context, VkSampler sampler, int width, int xresolution, int height, int yresolution, ITexture2 basetexture, std::vector<ITexture2> textures);
void Terrain_Destroy(ITerrain terrain);
