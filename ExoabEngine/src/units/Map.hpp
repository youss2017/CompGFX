#pragma once
#include "Entity.hpp"
#include "../graphics/RenderState.hpp"
#include "../graphics/material_system/ShaderProgramData.hpp"
#include "../core/pipeline/Pipeline.hpp"
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
    IShaderProgramData m_program_data;
    IBuffer2 m_vertices_buffer;
    IBuffer2 m_indices_buffer;
    RenderState m_render_state;
    EntityBindingData m_binding_data = EntityBindingData(0, 0);
    PipelineVertexInputDescription m_input_description;
    // TODO: Put Texture Map, Normal Map, Specular Map here
};

typedef Terrain* ITerrain;

// integers will round up to the nearest even number
// resolution refers to the amount of points between each vertices
ITerrain Terrain_Create(IPipelineLayout layout, int width, int xresolution, int height, int yresolution, IGPUTextureSampler sampler, IGPUTexture2D basetexture, int setID, int bindingID);
void Terrain_Destroy(ITerrain terrain);
