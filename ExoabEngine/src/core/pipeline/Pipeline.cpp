#include "Pipeline.hpp"
#include "../backend/VkGraphicsCard.hpp"
#include "../../utils/Logger.hpp"
#include <cassert>
#include <string>
#include <array>

#ifdef _DEBUG
static const bool s_DebugMode = true;
#else
static const bool s_DebugMode = false;
#endif

static bool PipelineLayout_Internal_VertexInputSafetyCheck(ShaderReflection *vertex_reflection, PipelineVertexInputDescription input_description)
{
    std::vector<uint32_t> attribute_locations = vertex_reflection->GetVertexAttributeLocations();
    assert(attribute_locations.size() == input_description.m_input_elements.size() && "Both shader and vertex input description must have the same amount of vertex shader input elements");
    if (attribute_locations.size() == 0)
        return true;

    // Make sure locations and type match
    for (int i = 0; i < attribute_locations.size(); i++)
    {
        uint32_t slocation = attribute_locations[i];
        auto Attribute = vertex_reflection->GetVertexAttribute(slocation);
        bool Found = false;
        for (auto &uelement : input_description.m_input_elements)
        {
            if (uelement.m_location == Attribute.m_location)
            {
                if (uelement.m_vector_size == Attribute.m_vecsize)
                {
                    if (uelement.m_IsFloatingPoint)
                    {
                        if (Attribute.m_type == spirv_cross::SPIRType::BaseType::Float) {
                            Found = true;
                            break;
                        }
                    }
                    else
                    {
                        if (uelement.m_IsUnsigned)
                        {
                            if (Attribute.m_type == spirv_cross::SPIRType::BaseType::UInt) {
                                Found = true;
                                break;
                            }
                            else
                                assert(0);
                        }
                        else
                        {
                            if (Attribute.m_type == spirv_cross::SPIRType::BaseType::Int) {
                                Found = true;
                                break;
                            }
                            else
                                assert(0);
                        }
                    }
                }
            }
        }
        if(!Found)
            return false;
    }

    // Make sure all strides match within each binding
    for (auto &srcelement : input_description.m_input_elements)
    {
        for (auto &dstelement : input_description.m_input_elements)
        {
            if (srcelement.m_binding_id == dstelement.m_binding_id)
            {
                if (srcelement.m_stride != dstelement.m_stride) {
                    logerror_break("Binding Strides do not match with other elements!");
                    return false;
                }
            }
        }
    }

    return true;
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ PIPELINE STATE ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

struct VulkanPipelineVertexInput
{
    VkPipelineVertexInputStateCreateInfo createInfo;
    std::vector<VkVertexInputBindingDescription> binding_descriptions;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    std::vector<VkVertexInputBindingDivisorDescriptionEXT> divisor_description;
    VkPipelineVertexInputDivisorStateCreateInfoEXT divisorCreateInfo;
};

static VulkanPipelineVertexInput Vulkan_Internal_PipelineState_InitalizeVertexInput(PipelineVertexInputDescription& input_description)
{
    VulkanPipelineVertexInput input_state;

    if(input_description.m_input_elements.size() > 0)
    {
        int lastBinding = -1;
        for(const auto& element : input_description.m_input_elements)
        {
            VkVertexInputAttributeDescription attribute;
            attribute.binding = element.m_binding_id;
            attribute.format = element.m_vk_format;
            attribute.location = element.m_location;
            attribute.offset = element.m_offset;
            input_state.attribute_descriptions.push_back(attribute);
            if(lastBinding != element.m_binding_id)
            {
                lastBinding = element.m_binding_id;
                VkVertexInputBindingDescription binding_description;
                binding_description.binding = element.m_binding_id;
                binding_description.inputRate = element.m_per_instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
                binding_description.stride = element.m_stride;
                if(element.m_per_instance)
                {
                    VkVertexInputBindingDivisorDescriptionEXT divisor_description;
                    divisor_description.binding = element.m_binding_id;
                    divisor_description.divisor = element.m_divisor_rate;
                    input_state.divisor_description.push_back(divisor_description);
                }
                input_state.binding_descriptions.push_back(binding_description);
            }
        }
    }

    input_state.createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    input_state.createInfo.pNext = nullptr;// input_state.divisor_description.size() > 0 ? &input_state.divisorCreateInfo : nullptr;
    input_state.createInfo.flags = 0;
    input_state.createInfo.vertexBindingDescriptionCount = input_state.binding_descriptions.size();
    input_state.createInfo.pVertexBindingDescriptions = input_state.binding_descriptions.data();
    input_state.createInfo.vertexAttributeDescriptionCount = input_state.attribute_descriptions.size();
    input_state.createInfo.pVertexAttributeDescriptions = input_state.attribute_descriptions.data();
    return input_state;
}

IPipelineState PipelineState_Create(GraphicsContext _context, const PipelineSpecification &spec, PipelineVertexInputDescription& input_description,
    Framebuffer fbo, VkPipelineLayout layout, Shader *vertex, Shader *fragment, const std::vector<VkDynamicState>& dynamicStates)
{
    vk::VkContext context = ToVKContext(_context);
    std::array<VkPipelineShaderStageCreateInfo, 2> Stages;
    Stages[0].sType = Stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    Stages[0].pNext = Stages[1].pNext = 0;
    Stages[0].flags = Stages[1].flags = 0;
    Stages[0].pSpecializationInfo = Stages[1].pSpecializationInfo = 0;
    Stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    Stages[0].module = (VkShaderModule)vertex->GetShader();
    Stages[0].pName = vertex->GetEntryPoint().c_str();
    VkSpecializationInfo vertexConstants = vertex->GetSpecializationInfo();
    VkSpecializationInfo fragmentConstants = fragment->GetSpecializationInfo();
    Stages[0].pSpecializationInfo = &vertexConstants;

    Stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    Stages[1].module = (VkShaderModule)fragment->GetShader();
    Stages[1].pName = fragment->GetEntryPoint().c_str();
    Stages[1].pSpecializationInfo = &fragmentConstants;

    VulkanPipelineVertexInput VertexInputState = Vulkan_Internal_PipelineState_InitalizeVertexInput(input_description);

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
    InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyState.pNext = nullptr;
    InputAssemblyState.flags = 0;
    if (PolygonTopology::TRIANGLE_LIST == spec.m_Topology)
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    else if (PolygonTopology::LINE_LIST == spec.m_Topology)
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    else if (PolygonTopology::POINT_LIST == spec.m_Topology)
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    else
        assert(0);
    InputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo TessellationState;
    TessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    TessellationState.pNext = nullptr;
    TessellationState.flags = 0;
    TessellationState.patchControlPoints = 1;

    // The viewport and scissor are based on the dimensions of the framebuffer.

    VkViewport viewport;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = fbo.m_width;
    viewport.height = fbo.m_height;
    viewport.minDepth = spec.m_NearField;
    viewport.maxDepth = spec.m_FarField;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = fbo.m_width;
    scissor.extent.height = fbo.m_height;

    VkPipelineViewportStateCreateInfo ViewportState;
    ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.pNext = nullptr;
    ViewportState.flags = 0;
    ViewportState.viewportCount = 1;
    ViewportState.pViewports = &viewport;
    ViewportState.scissorCount = 1;
    ViewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo RasterizationState;
    RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizationState.pNext = nullptr;
    RasterizationState.flags = 0;
    RasterizationState.depthClampEnable = VK_FALSE;
    RasterizationState.rasterizerDiscardEnable = VK_FALSE;
    if (spec.m_PolygonMode == PolygonMode::FILL)
        RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    else if (spec.m_PolygonMode == PolygonMode::LINE)
        RasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
    else if (spec.m_PolygonMode == PolygonMode::POINT)
        RasterizationState.polygonMode = VK_POLYGON_MODE_POINT;
    if (spec.m_CullMode == CullMode::CULL_BACK)
        RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    else if (spec.m_CullMode == CullMode::CULL_FRONT)
        RasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    else if (spec.m_CullMode == CullMode::CULL_FRONT_AND_BACK)
        RasterizationState.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
    else if (spec.m_CullMode == CullMode::CULL_NONE)
        RasterizationState.cullMode = VK_CULL_MODE_NONE;
    RasterizationState.frontFace = (spec.m_FrontFaceCCW) ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    RasterizationState.depthBiasEnable = VK_FALSE;
    RasterizationState.depthBiasConstantFactor = 0.0f;
    RasterizationState.depthBiasClamp = 0.0f;
    RasterizationState.depthBiasSlopeFactor = 0.0f;
    RasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo MultisampleState;
    MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisampleState.pNext = nullptr;
    MultisampleState.flags = 0;
    MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    // TODO: Determine if sample shading is useful and makes a visual difference
    MultisampleState.sampleShadingEnable = VK_FALSE;
    MultisampleState.minSampleShading = 0.0f;
    MultisampleState.pSampleMask = nullptr;
    MultisampleState.alphaToCoverageEnable = VK_FALSE;
    MultisampleState.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo DepthStencilState;
    DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilState.pNext = nullptr;
    DepthStencilState.flags = 0;
    DepthStencilState.depthTestEnable = spec.m_DepthEnabled;
    DepthStencilState.depthWriteEnable = spec.m_DepthWriteEnable;
    if (spec.m_DepthFunc == DepthFunction::ALWAYS)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    else if (spec.m_DepthFunc == DepthFunction::EQUAL)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_EQUAL;
    else if (spec.m_DepthFunc == DepthFunction::GREATER)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;
    else if (spec.m_DepthFunc == DepthFunction::GREATER_EQUAL)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    else if (spec.m_DepthFunc == DepthFunction::LESS)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    else if (spec.m_DepthFunc == DepthFunction::LESS_EQUAL)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    else if (spec.m_DepthFunc == DepthFunction::NEVER)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_NEVER;
    else if (spec.m_DepthFunc == DepthFunction::NOT_EQUAL)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
    DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    DepthStencilState.stencilTestEnable = VK_FALSE;
    DepthStencilState.front = {};
    DepthStencilState.back = {};
    DepthStencilState.minDepthBounds = 0.0f;
    DepthStencilState.maxDepthBounds = 1.0f;

    std::vector<VkPipelineColorBlendAttachmentState> BlendAttachmentStates(fbo.m_color_attachments.size());
    for (int i = 0; i < BlendAttachmentStates.size(); i++) {
        BlendAttachmentStates[i] = fbo.m_color_attachments[i].GetBlendState();
    }

    VkPipelineColorBlendStateCreateInfo ColorBlendState;
    ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendState.pNext = nullptr;
    ColorBlendState.flags = 0;
    ColorBlendState.logicOpEnable = VK_FALSE;
    ColorBlendState.logicOp = VK_LOGIC_OP_MAX_ENUM;
    ColorBlendState.attachmentCount = BlendAttachmentStates.size();
    ColorBlendState.pAttachments = BlendAttachmentStates.data();
    ColorBlendState.blendConstants[0] = 0.0;
    ColorBlendState.blendConstants[1] = 0.0;
    ColorBlendState.blendConstants[2] = 0.0;
    ColorBlendState.blendConstants[3] = 0.0;

    VkPipelineDynamicStateCreateInfo DynamicState;
    DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.pNext = nullptr;
    DynamicState.flags = 0;
    DynamicState.dynamicStateCount = dynamicStates.size();
    DynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo dynamicCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    dynamicCreateInfo.viewMask = 0;
    dynamicCreateInfo.colorAttachmentCount = fbo.m_color_attachments.size();
    std::vector<VkFormat> colorFormat(dynamicCreateInfo.colorAttachmentCount);
    for (int i = 0; i < colorFormat.size(); i++)
        colorFormat[i] = fbo.m_color_attachments[i].GetFormat();
    if (colorFormat.size() > 0) {
        dynamicCreateInfo.pColorAttachmentFormats = &colorFormat[0];
    }
    else {
        dynamicCreateInfo.pColorAttachmentFormats = nullptr;
    }
    if (fbo.m_depth_attachment.has_value()) {
        dynamicCreateInfo.depthAttachmentFormat = fbo.m_depth_attachment->GetFormat();
        dynamicCreateInfo.stencilAttachmentFormat = fbo.m_depth_attachment->GetFormat();
    }

    VkGraphicsPipelineCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext = &dynamicCreateInfo;
    createInfo.flags = (s_DebugMode) ? (VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT | VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR) : (0);
    createInfo.stageCount = 2;
    createInfo.pStages = Stages.data();
    createInfo.pVertexInputState = &VertexInputState.createInfo;
    createInfo.pInputAssemblyState = &InputAssemblyState;
    createInfo.pTessellationState = &TessellationState;
    createInfo.pViewportState = &ViewportState;
    createInfo.pRasterizationState = &RasterizationState;
    createInfo.pMultisampleState = &MultisampleState;
    createInfo.pDepthStencilState = &DepthStencilState;
    createInfo.pColorBlendState = &ColorBlendState;
    createInfo.pDynamicState = nullptr;
    createInfo.layout = layout;
    createInfo.renderPass = nullptr;
    createInfo.subpass = 0;
    createInfo.basePipelineHandle = 0;
    createInfo.basePipelineIndex = 0;

    auto vcont = ToVKContext(context);
    VkPipeline pipeline;
    vkcheck(vkCreateGraphicsPipelines(vcont->defaultDevice, nullptr, 1, &createInfo, 0, &pipeline));

    PipelineState *state = new PipelineState();
    state->m_ApiType = 0;
    state->m_context = context;
    state->m_pipeline = pipeline;
    state->m_spec = spec;
    state->m_layout = layout;

    return state;
}

void PipelineState_Destroy(IPipelineState state)
{
    auto vcont = ToVKContext(state->m_context);
    vkDestroyPipeline(vcont->defaultDevice, (VkPipeline)state->m_pipeline, 0);
    delete state;
}
