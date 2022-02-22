#pragma once
#include "../core/pipeline/Pipeline.hpp"
#include <memory/Buffer2.hpp>
#include <memory/Texture2.hpp>
#include <shaders/ShaderBinding.hpp>
#include <vector>
#include <glm/glm.hpp>

struct TerrainVertex
{
    glm::lowp_fvec3 inPosition;
    glm::u8vec3 inNormal;
    glm::u8vec3 inTextureIDs;
    glm::u8vec3 inTextureWeights;
    glm::lowp_fvec2 inTexCoords;
};

struct Terrain
{
    GraphicsContext m_context;
    int m_width, m_height;
    std::vector<TerrainVertex> m_vertices;
    std::vector<uint16_t> m_indices;
    IBuffer2 m_vertices_ssbo;
    IBuffer2 m_indices_buffer;
    ShaderSet m_Set0;
    ShaderSet m_Set1;
    VkDescriptorPool m_pool;
};

typedef Terrain* ITerrain;

ITerrain Terrain_Create(GraphicsContext context, VkPipelineLayout layout, VkSampler sampler, int width, int xresolution, int height, int yresolution, ITexture2 basetexture, std::vector<ITexture2> textures);
void Terrain_Destroy(ITerrain terrain);
