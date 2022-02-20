#pragma once
#include "../shaders/Shader.hpp"
#include "../shaders/ShaderReflection.hpp"
#include "Framebuffer.hpp"
#include <algorithm>

struct PipelineVertexInputDescription
{
    struct PipelineVertexInputElement
    {
        std::string m_name;
        uint32_t m_location;
        uint32_t m_binding_id;
        uint32_t m_offset;
        uint32_t m_stride;
        uint32_t m_vector_size;
        uint32_t m_size;
        uint32_t m_divisor_rate;
        bool m_IsFloatingPoint;
        bool m_IsUnsigned;
        bool m_per_instance;
        VkFormat m_vk_format;
    };

    // bindingID is used for instancing
    // set offset to UINT32_MAX if you want me to calculate offset (same for stride)
    void AddInputElement(const std::string &name, uint32_t location, uint32_t binding_id, uint32_t vector_size, bool isfloatingpoint, bool isunsigned, bool per_instance, uint32_t divisor_rate = 1)
    {
        PipelineVertexInputElement element;
        element.m_name = name;
        element.m_location = location;
        element.m_binding_id = binding_id;
        element.m_vector_size = vector_size;
        element.m_size = vector_size * 4;
        element.m_divisor_rate = divisor_rate;
        element.m_IsFloatingPoint = isfloatingpoint;
        element.m_IsUnsigned = isunsigned;
        element.m_per_instance = per_instance;

        if (binding_id == 0 && per_instance)
        {
            logerror_break("Binding 0 MUST be per vertex!!!!");
        }

        // Were assuming these are vector types and they are either
        // vec(N), ivec(N), uvec(N) which are all 4*N bytes wide.

        element.m_offset = 0;
        for (auto &e : m_input_elements)
            if (e.m_binding_id == binding_id)
                element.m_offset += e.m_vector_size * 4;

        uint32_t cstride = element.m_size;
        for (auto &e : m_input_elements)
            if (e.m_binding_id == binding_id)
                cstride += e.m_size;
        for (auto &e : m_input_elements)
            if (e.m_binding_id == binding_id)
                e.m_stride = cstride;
        element.m_stride = cstride;

        // make sure there are no duplicates and all divisors are the same
        for (auto &e : m_input_elements)
        {
            if (e.m_binding_id == binding_id)
            {
                if (e.m_location == location)
                {
                    assert(0 && "AddInputElement element already exists!");
                }
                if (e.m_divisor_rate != divisor_rate)
                {
                    logerror_break("VertexInputDescription elements within same bindings have different divisor rates. ALL elements within binding must have the same divisor rate!");
                }
            }
        }

        if (element.m_IsFloatingPoint)
        {
            switch (element.m_vector_size)
            {
            case 1:
                element.m_vk_format = VK_FORMAT_R32_SFLOAT;
                break;
            case 2:
                element.m_vk_format = VK_FORMAT_R32G32_SFLOAT;
                break;
            case 3:
                element.m_vk_format = VK_FORMAT_R32G32B32_SFLOAT;
                break;
            case 4:
                element.m_vk_format = VK_FORMAT_R32G32B32A32_SFLOAT;
                break;
            default:
                assert(0);
            }
        }
        else
        {
            switch (element.m_vector_size)
            {
            case 1:
                element.m_vk_format = element.m_IsUnsigned ? VK_FORMAT_R32_UINT : VK_FORMAT_R32_SINT;
                break;
            case 2:
                element.m_vk_format = element.m_IsUnsigned ? VK_FORMAT_R32G32_UINT : VK_FORMAT_R32G32_SINT;
                break;
            case 3:
                element.m_vk_format = element.m_IsUnsigned ? VK_FORMAT_R32G32B32_UINT : VK_FORMAT_R32G32B32_SINT;
                break;
            case 4:
                element.m_vk_format = element.m_IsUnsigned ? VK_FORMAT_R32G32B32A32_UINT : VK_FORMAT_R32G32B32A32_SINT;
                break;
            default:
                assert(0);
            }
        }

        m_input_elements.push_back(element);

        // sort by bindingID
        auto sortingFunction = [](PipelineVertexInputElement const &lhs, PipelineVertexInputElement const &rhs) throw()->bool
        {
            if (lhs.m_binding_id > rhs.m_binding_id)
                return false;
            if (lhs.m_location > rhs.m_location)
                return false;
            return true;
        };
        std::sort(m_input_elements.begin(), m_input_elements.end(), sortingFunction);
    }

    uint32_t GetDivisorRate(uint32_t bindingID)
    {
        for (auto &e : m_input_elements)
            if (e.m_binding_id == bindingID)
                return e.m_divisor_rate;
        return UINT32_MAX;
    }

    std::vector<PipelineVertexInputElement> m_input_elements;
};

enum class CullMode
{
    CULL_NONE,
    CULL_FRONT,
    CULL_BACK,
    CULL_FRONT_AND_BACK
};

enum class DepthFunction
{
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS
};

enum class PolygonMode
{
    FILL,
    LINE,
    POINT
};

enum class PolygonTopology
{
    TRIANGLE_LIST,
    LINE_LIST,
    POINT_LIST
};

struct PipelineSpecification
{
    CullMode m_CullMode;
    bool m_DepthEnabled, m_DepthWriteEnable;
    DepthFunction m_DepthFunc;
    PolygonMode m_PolygonMode;
    bool m_FrontFaceCCW; // Is the front face Counter Clockwise (TRUE) or Clockwise (FALSE)
    PolygonTopology m_Topology;
    bool m_SampleRateShadingEnabled;
    // from 0.0f to 1.0f
    float m_MinSampleShading;
    float m_NearField = 0.0f;
    float m_FarField = 1.0f;
};

struct PipelineState
{
    int m_ApiType;
    GraphicsContext m_context;
    APIHandle m_pipeline;
    PipelineSpecification m_spec;
    VkPipelineLayout m_layout;
    IFramebufferStateManagement m_StateManagment;
};

typedef PipelineState *IPipelineState;

typedef IPipelineState PFN_PipelineState_Create(GraphicsContext context, const PipelineSpecification &spec, FramebufferStateManagement *StateManagment, PipelineVertexInputDescription& input_description, VkPipelineLayout layout, Shader *vertex, Shader *fragment);
typedef void PFN_PipelineState_Destroy(IPipelineState state);

extern PFN_PipelineState_Create *PipelineState_Create;
extern PFN_PipelineState_Destroy *PipelineState_Destroy;

void PipelineState_LinkFunctions(GraphicsContext context);
