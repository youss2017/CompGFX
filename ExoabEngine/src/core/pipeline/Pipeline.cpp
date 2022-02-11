#include "Pipeline.hpp"
#include "../backend/VkGraphicsCard.hpp"
#include "../backend/GLGraphicsCard.hpp"
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

//////////////////////////////////////// PIPELINE LAYOUT

PFN_PipelineLayout_Create *PipelineLayout_Create = nullptr;
PFN_PipelineLayout_Destroy *PipelineLayout_Destroy = nullptr;

IPipelineLayout Vulkan_PipelineLayout_Create(GraphicsContext context, const PipelineVertexInputDescription &input_description, ShaderReflection *vertex_reflection, ShaderReflection *fragment_reflection)
{
    assert(context && vertex_reflection && fragment_reflection);
    ShaderReflection combined = ShaderReflection::CombineReflections(vertex_reflection, fragment_reflection);
    auto &set_descs = combined.GetShaderSetDescriptions();
    std::vector<VkPushConstantRange> pushconstants = ShaderReflection_CombinePushconstantRanges(vertex_reflection, fragment_reflection);
    auto vcont = ToVKContext(context);
    std::vector<VkDescriptorSetLayout> setlayouts;
    std::vector<VkDescriptorSetLayout> real_setlayouts;

    if (!PipelineLayout_Internal_VertexInputSafetyCheck(vertex_reflection, input_description))
    {
        logerror_break("PipelineVertexInputDescription does not match actual shader vertex input layout");
        return nullptr;
    }

    auto CreateEmptySetLayout = [](VkDevice device, VkAllocationCallbacks * pCallbacks) throw()->VkDescriptorSetLayout
    {
        VkDescriptorSetLayoutCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.bindingCount = 0;
        createInfo.pBindings = nullptr;
        VkDescriptorSetLayout set_layout;
        vkcheck(vkCreateDescriptorSetLayout(device, &createInfo, pCallbacks, &set_layout));
        return set_layout;
    };

    if (set_descs.size() > 0)
        for (uint32_t w = 0; w < set_descs[0].m_SetID; w++)
        {
            setlayouts.push_back(CreateEmptySetLayout(vcont->defaultDevice, vcont->m_allocation_callback));
        }

    uint32_t ii = 0;
    for (auto &set_desc : set_descs)
    {
        VkDescriptorSetLayoutBinding pBindings[250];
        int binding_index = 0;
        for (auto &binding_desc : set_desc.m_UniformBuffers)
        {
            pBindings[binding_index].binding = binding_desc.m_Binding;
            pBindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            pBindings[binding_index].descriptorCount = binding_desc.m_ArraySize;
            pBindings[binding_index].stageFlags = ShaderReflection_GetVulkanShaderStage(binding_desc.m_shaderStage);
            pBindings[binding_index].pImmutableSamplers = nullptr;
            binding_index++;
        }

        for (auto &binding_desc : set_desc.m_SampledImage)
        {
            pBindings[binding_index].binding = binding_desc.m_Binding;
            pBindings[binding_index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            pBindings[binding_index].descriptorCount = binding_desc.m_ArraySize;
            pBindings[binding_index].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pBindings[binding_index].pImmutableSamplers = nullptr;
            binding_index++;
        }

        VkDescriptorSetLayoutCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.bindingCount = set_desc.m_BindingCount;
        createInfo.pBindings = pBindings;
        VkDescriptorSetLayout SetLayout;
        vkcheck(vkCreateDescriptorSetLayout(vcont->defaultDevice, &createInfo, vcont->m_allocation_callback, &SetLayout));
        setlayouts.push_back(SetLayout);
        real_setlayouts.push_back(SetLayout);
        ii++;
    }

    VkPipelineLayoutCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.setLayoutCount = setlayouts.size();
    createInfo.pSetLayouts = setlayouts.data();
    createInfo.pushConstantRangeCount = pushconstants.size();
    createInfo.pPushConstantRanges = pushconstants.data();
    VkPipelineLayout layout;
    vkCreatePipelineLayout(vcont->defaultDevice, &createInfo, vcont->m_allocation_callback, &layout);

    IPipelineLayout pipeline_layout = new PipelineLayout();
    pipeline_layout->m_ApiType = 0;
    pipeline_layout->m_context = context;
    pipeline_layout->m_vertex_input_description = input_description;
    pipeline_layout->m_pipelinelayout = (APIHandle)layout;
    pipeline_layout->m_setlayouts = setlayouts;
    pipeline_layout->m_real_setlayouts = real_setlayouts;
    pipeline_layout->m_vertex_reflection = *vertex_reflection;
    pipeline_layout->m_fragment_reflection = *fragment_reflection;
    pipeline_layout->m_set_descs = set_descs;
    return pipeline_layout;
}

void Vulkan_PipelineLayout_Destroy(IPipelineLayout layout)
{
    auto vcont = ToVKContext(layout->m_context);
    vkDestroyPipelineLayout(vcont->defaultDevice, (VkPipelineLayout)layout->m_pipelinelayout, vcont->m_allocation_callback);
    for (const auto &setlayout : layout->m_setlayouts)
        vkDestroyDescriptorSetLayout(vcont->defaultDevice, setlayout, vcont->m_allocation_callback);
    delete layout;
}

IPipelineLayout GL_PipelineLayout_Create(GraphicsContext context, const PipelineVertexInputDescription &input_description, ShaderReflection *vertex_reflection, ShaderReflection *fragment_reflection)
{
    assert(context && vertex_reflection && fragment_reflection);
    ShaderReflection combined = ShaderReflection::CombineReflections(vertex_reflection, fragment_reflection);
    auto &set_descs = combined.GetShaderSetDescriptions();
    std::vector<VkPushConstantRange> pushconstants = ShaderReflection_CombinePushconstantRanges(vertex_reflection, fragment_reflection);
    auto vcont = ToVKContext(context);
    std::vector<VkDescriptorSetLayout> setlayouts;
    std::vector<VkDescriptorSetLayout> real_setlayouts;

    if (!PipelineLayout_Internal_VertexInputSafetyCheck(vertex_reflection, input_description))
    {
        log_error("PipelineVertexInputDescription does not match actual shader vertex input layout", __FILE__, __LINE__);
        return nullptr;
    }

    // TODO: Figure out opengl shader system
    // Use uniform block objects if GL_ARB_uniform_buffer_object is supported otherwise use plain uniforms.
#if 0
        for(const auto& set_desc : set_descs) {
            for(auto& buf : set_desc.m_UniformBuffers) {
                uint32_t bufferID;
                glGenBuffers(1, &bufferID);
                glBindBuffer(GL_UNIFORM_BUFFER, bufferID);
                glBufferData(GL_UNIFORM_BUFFER, buf.m_Size * buf.m_ArraySize, nullptr, GL_DYNAMIC_DRAW);
            }
        }
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
#endif
    IPipelineLayout pipeline_layout = new PipelineLayout();
    pipeline_layout->m_ApiType = 1;
    pipeline_layout->m_context = context;
    pipeline_layout->m_vertex_input_description = input_description;
    return pipeline_layout;
}

void GL_PipelineLayout_Destroy(IPipelineLayout layout)
{
    delete layout;
}

void PipelineLayout_LinkFunctions(GraphicsContext context)
{
    char ApiType = *(char *)context;
    if (ApiType == 0)
    {
        PipelineLayout_Create = Vulkan_PipelineLayout_Create;
        PipelineLayout_Destroy = Vulkan_PipelineLayout_Destroy;
    }
    else if (ApiType == 1)
    {
        PipelineLayout_Create = GL_PipelineLayout_Create;
        PipelineLayout_Destroy = GL_PipelineLayout_Destroy;
    }
    else
    {
        Utils::Break();
    }
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

static VulkanPipelineVertexInput Vulkan_Internal_PipelineState_InitalizeVertexInput(IPipelineLayout layout)
{
    VulkanPipelineVertexInput input_state;

    PipelineVertexInputDescription &input_description = layout->m_vertex_input_description;
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

IPipelineState Vulkan_PipelineState_Create(GraphicsContext _context, const PipelineSpecification &spec, FramebufferStateManagement *StateManagment, IPipelineLayout layout, Shader *vertex, Shader *fragment)
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

    Stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    Stages[1].module = (VkShaderModule)fragment->GetShader();
    Stages[1].pName = fragment->GetEntryPoint().c_str();

    VulkanPipelineVertexInput VertexInputState = Vulkan_Internal_PipelineState_InitalizeVertexInput(layout);

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
    viewport.width = 1;
    viewport.height = 1;
    viewport.minDepth = spec.m_NearField;
    viewport.maxDepth = spec.m_FarField;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = 1;
    scissor.extent.height = 1;

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
    MultisampleState.rasterizationSamples = (VkSampleCountFlagBits)StateManagment->m_Samples;
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

    int BlendAttachmentCount = StateManagment->m_ColorAttachmentCount;
    VkPipelineColorBlendAttachmentState *pBlendAttachmentStates = (VkPipelineColorBlendAttachmentState *)alloca(sizeof(VkPipelineColorBlendAttachmentState) * BlendAttachmentCount);

    auto IsAttachmentColor = [](const FramebufferAttachment &attachment) throw()->bool
    {
        if (attachment.m_format == TextureFormat::D32F)
            return false;
        return true;
    };
    const auto &state_managment_attachments = StateManagment->m_attachments;

    for (int i = 0, j = 0; j < BlendAttachmentCount; i++)
    {
        if (IsAttachmentColor(state_managment_attachments[i]))
        {
            pBlendAttachmentStates[j++] = state_managment_attachments[i].BlendState;
        }
    }

    VkPipelineColorBlendStateCreateInfo ColorBlendState;
    ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendState.pNext = nullptr;
    ColorBlendState.flags = 0;
    ColorBlendState.logicOpEnable = VK_FALSE;
    ColorBlendState.logicOp = VK_LOGIC_OP_MAX_ENUM;
    ColorBlendState.attachmentCount = BlendAttachmentCount;
    ColorBlendState.pAttachments = pBlendAttachmentStates;
    ColorBlendState.blendConstants[0] = StateManagment->m_BlendConstantRed;
    ColorBlendState.blendConstants[1] = StateManagment->m_BlendConstantGreen;
    ColorBlendState.blendConstants[2] = StateManagment->m_BlendConstantBlue;
    ColorBlendState.blendConstants[3] = StateManagment->m_BlendConstantAlpha;

    VkPipelineDynamicStateCreateInfo DynamicState;
    std::array<VkDynamicState, 2> DynamicStates;
    DynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
    DynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;
    DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.pNext = nullptr;
    DynamicState.flags = 0;
    DynamicState.dynamicStateCount = DynamicStates.size();
    DynamicState.pDynamicStates = DynamicStates.data();

    VkGraphicsPipelineCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext = nullptr;
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
    createInfo.pDynamicState = &DynamicState;
    createInfo.layout = (VkPipelineLayout)layout->m_pipelinelayout;
    createInfo.renderPass = (VkRenderPass)StateManagment->m_renderpass;
    createInfo.subpass = 0; // No subpass support
    createInfo.basePipelineHandle = 0;
    createInfo.basePipelineIndex = 0;

    auto vcont = ToVKContext(context);
    VkPipeline pipeline;
    vkcheck(vkCreateGraphicsPipelines(vcont->defaultDevice, nullptr, 1, &createInfo, vcont->m_allocation_callback, &pipeline));

    PipelineState *state = new PipelineState();
    state->m_ApiType = 0;
    state->m_context = context;
    state->m_pipeline = pipeline;
    state->m_spec = spec;
    state->m_StateManagment = StateManagment;
    state->m_layout = layout;

    return state;
}

void Vulkan_PipelineState_Destroy(IPipelineState state)
{
    auto vcont = ToVKContext(state->m_context);
    vkDestroyPipeline(vcont->defaultDevice, (VkPipeline)state->m_pipeline, vcont->m_allocation_callback);
    delete state;
}

// ===================================== OPENGL =====================================

IPipelineState GL_PipelineState_Create(GraphicsContext context, const PipelineSpecification &spec, FramebufferStateManagement *StateManagment, IPipelineLayout layout, Shader *vertex, Shader *fragment)
{
    GLuint programID = glCreateProgram();
    glAttachShader(programID, PVOIDToUInt32(vertex->GetShader()));
    glAttachShader(programID, PVOIDToUInt32(fragment->GetShader()));
    glLinkProgram(programID);
    int link_status;
    glGetProgramiv(programID, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE)
    {
        char log[512];
        glGetProgramInfoLog(programID, 512, nullptr, log);
        std::string err_msg = "Could not link (GL) program! Error Message:\n" + std::string(log);
        log_error(err_msg.c_str());
        return nullptr;
    }
    PipelineState *state = new PipelineState();
    state->m_ApiType = 1;
    state->m_context = context;
    state->m_pipeline = UInt32ToPVOID(programID);
    state->m_spec = spec;
    state->m_StateManagment = StateManagment;
    state->m_layout = layout;

    return state;
}

void GL_PipelineState_Destroy(IPipelineState state)
{
    glDeleteProgram(PVOIDToUInt32(state->m_pipeline));
    delete state;
}

PFN_PipelineState_Create *PipelineState_Create = nullptr;
PFN_PipelineState_Destroy *PipelineState_Destroy = nullptr;

void PipelineState_LinkFunctions(GraphicsContext context)
{
    char ApiType = *(char *)context;
    if (ApiType == 0)
    {
        PipelineState_Create = Vulkan_PipelineState_Create;
        PipelineState_Destroy = Vulkan_PipelineState_Destroy;
    }
    else if (ApiType == 1)
    {
        PipelineState_Create = GL_PipelineState_Create;
        PipelineState_Destroy = GL_PipelineState_Destroy;
    }
    else
    {
        Utils::Break();
    }
}
