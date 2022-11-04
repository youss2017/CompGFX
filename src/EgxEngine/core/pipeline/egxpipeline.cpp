#include "egxpipeline.hpp"
#include <cassert>

egx::ref<egx::Pipeline>  egx::Pipeline::FactoryCreate(const ref<VulkanCoreInterface>& CoreInterface)
{
    return ref<Pipeline>(new Pipeline(CoreInterface));
}

egx::Pipeline::~Pipeline()
{
    if (Pipe) {
        vkDestroyPipeline(_coreinterface->Device, Pipe, nullptr);
    }
}

egx::Pipeline::Pipeline(Pipeline&& move) noexcept :
    _coreinterface(move._coreinterface)
{
    memcpy(this, &move, sizeof(Pipeline));
    memset(&move, 0, sizeof(Pipeline));
}

 egx::Pipeline& egx::Pipeline::operator=(Pipeline&& move) noexcept
{
    memcpy(this, &move, sizeof(Pipeline));
    memset(&move, 0, sizeof(Pipeline));
    return *this;
}

void  egx::Pipeline::invalidate(
    const ref<PipelineLayout>& layout,
    const egxshader& vertex, 
    const egxshader& fragment, 
    const ref<Framebuffer>& framebuffer,
    const uint32_t PassId,
    const egxvertexdescription& vertexDescription)
{
    if (Pipe) vkDestroyPipeline(_coreinterface->Device, Pipe, nullptr);
    _graphics = true;
    VkPipelineShaderStageCreateInfo Stages[2]{};
    Stages[0].sType = Stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    Stages[0].pNext = Stages[1].pNext = 0;
    Stages[0].flags = Stages[1].flags = 0;
    Stages[0].pSpecializationInfo = Stages[1].pSpecializationInfo = 0;
    Stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    Stages[0].module = vertex.GetShader();
    Stages[0].pName = vertex.GetEntryPoint().c_str();
    VkSpecializationInfo vertexConstants = vertex.GetSpecializationInfo();
    VkSpecializationInfo fragmentConstants = fragment.GetSpecializationInfo();
    Stages[0].pSpecializationInfo = &vertexConstants;

    Stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    Stages[1].module = fragment.GetShader();
    Stages[1].pName = fragment.GetEntryPoint().c_str();
    Stages[1].pSpecializationInfo = &fragmentConstants;

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState{};
    InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyState.pNext = nullptr;
    InputAssemblyState.flags = 0;
    if (polgyontopology_trianglelist == Topology)
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    else if (polgyontopology_linelist == Topology)
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    else if (polgyontopology_linestrip == Topology)
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    else if (polgyontopology_pointlist == Topology)
        InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    else
        assert(0);
    InputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo TessellationState{};
    TessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    TessellationState.pNext = nullptr;
    TessellationState.flags = 0;
    TessellationState.patchControlPoints = 1;

    // The viewport and scissor are based on the dimensions of the framebuffer.

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = float(ViewportWidth == 0 ? framebuffer->Width : ViewportWidth);
    viewport.height = float(ViewportHeight == 0 ? framebuffer->Height : ViewportHeight);
    viewport.minDepth = NearField;
    viewport.maxDepth = FarField;

    VkRect2D scissor{};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width  = uint32_t(viewport.width);
    scissor.extent.height = uint32_t(viewport.height);

    VkPipelineViewportStateCreateInfo ViewportState{};
    ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.pNext = nullptr;
    ViewportState.flags = 0;
    ViewportState.viewportCount = 1;
    ViewportState.pViewports = &viewport;
    ViewportState.scissorCount = 1;
    ViewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo RasterizationState{};
    RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizationState.pNext = nullptr;
    RasterizationState.flags = 0;
    RasterizationState.depthClampEnable = VK_FALSE;
    RasterizationState.rasterizerDiscardEnable = VK_FALSE;
    if (FillMode == polygonmode_fill)
        RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    else if (FillMode == polygonmode_line)
        RasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
    else if (FillMode == polygonmode_point)
        RasterizationState.polygonMode = VK_POLYGON_MODE_POINT;
    if (CullMode == cullmode_back)
        RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    else if (CullMode == cullmode_front)
        RasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
    else if (CullMode == cullmode_front_and_back)
        RasterizationState.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
    else if (CullMode == cullmode_none)
        RasterizationState.cullMode = VK_CULL_MODE_NONE;
    RasterizationState.frontFace = FrontFace == frontface_ccw ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
    RasterizationState.depthBiasEnable = VK_FALSE;
    RasterizationState.depthBiasConstantFactor = 0.0f;
    RasterizationState.depthBiasClamp = 0.0f;
    RasterizationState.depthBiasSlopeFactor = 0.0f;
    RasterizationState.lineWidth = LineWidth;

    VkPipelineMultisampleStateCreateInfo MultisampleState{};
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

    VkPipelineDepthStencilStateCreateInfo DepthStencilState{};
    DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilState.pNext = nullptr;
    DepthStencilState.flags = 0;
    DepthStencilState.depthTestEnable = DepthEnabled;
    DepthStencilState.depthWriteEnable = DepthWriteEnable;
    if (DepthCompare == depthcompare_always)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    else if (DepthCompare == depthcompare_equal)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_EQUAL;
    else if (DepthCompare == depthcompare_greater)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;
    else if (DepthCompare == depthcompare_greater_equal)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    else if (DepthCompare == depthcompare_less)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    else if (DepthCompare == depthcompare_less_equal)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    else if (DepthCompare == depthcompare_never)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_NEVER;
    else if (DepthCompare == depthcompare_not_equal)
        DepthStencilState.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
    DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    DepthStencilState.stencilTestEnable = VK_FALSE;
    DepthStencilState.front = {};
    DepthStencilState.back = {};
    DepthStencilState.minDepthBounds = 0.0f;
    DepthStencilState.maxDepthBounds = 1.0f;

    std::vector<VkPipelineColorBlendAttachmentState> BlendAttachmentStates(framebuffer->_colorattachements.size());
    {
        int i = 0;
        for (const auto&[Id, attachment] : framebuffer->_colorattachements) {
            BlendAttachmentStates[i] = attachment.GetBlendState();
            i++;
        }
    }

    VkPipelineColorBlendStateCreateInfo ColorBlendState{};
    ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendState.pNext = nullptr;
    ColorBlendState.flags = 0;
    ColorBlendState.logicOpEnable = VK_FALSE;
    ColorBlendState.logicOp = VK_LOGIC_OP_MAX_ENUM;
    ColorBlendState.attachmentCount = (uint32_t)BlendAttachmentStates.size();
    ColorBlendState.pAttachments = BlendAttachmentStates.data();
    ColorBlendState.blendConstants[0] = 0.0;
    ColorBlendState.blendConstants[1] = 0.0;
    ColorBlendState.blendConstants[2] = 0.0;
    ColorBlendState.blendConstants[3] = 0.0;

    VkPipelineDynamicStateCreateInfo DynamicState{};
    DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.pNext = nullptr;
    DynamicState.flags = 0;
    DynamicState.dynamicStateCount = 0;
    DynamicState.pDynamicStates = nullptr;

#if 0
    VkPipelineRenderingCreateInfo dynamicCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    dynamicCreateInfo.viewMask = 0;
    dynamicCreateInfo.colorAttachmentCount = (uint32_t)framebuffer->_colorattachements.size();
    std::vector<VkFormat> colorFormat(dynamicCreateInfo.colorAttachmentCount);
    {
        int i = 0;
        for (const auto& [id, attachment] : framebuffer->_colorattachements) {
            colorFormat[i] = attachment.Attachment->Format;
            i++;
        }
    }
    if (colorFormat.size() > 0) {
        dynamicCreateInfo.pColorAttachmentFormats = &colorFormat[0];
    }
    else {
        dynamicCreateInfo.pColorAttachmentFormats = nullptr;
    }
    if (framebuffer->_depthattachment.has_value()) {
        dynamicCreateInfo.depthAttachmentFormat = framebuffer->_depthattachment->Attachment->Format;
        dynamicCreateInfo.stencilAttachmentFormat = framebuffer->_depthattachment->Attachment->Format;
    }
#endif

    VkGraphicsPipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext = nullptr; //&dynamicCreateInfo;
#ifdef _DEBUG
    createInfo.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT | VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
#endif
    VkPipelineVertexInputStateCreateInfo VertexInputState{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    std::vector<VkVertexInputAttributeDescription> VertexAttributes;
    std::vector<VkVertexInputBindingDescription> VertexBinding;
    {
        auto bindingList = vertexDescription.GetBindings();
        VertexBinding.resize(bindingList.size());
        int i = 0;
        for (uint32_t bindingId : bindingList) {
            VertexBinding[i].binding = bindingId;
            VertexBinding[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            VertexBinding[i].stride = vertexDescription.GetStride(bindingId);
            i++;
        }
        for (auto& e : vertexDescription.Attributes) {
            VkVertexInputAttributeDescription attribute{};
            attribute.binding = e.BindingId;
            attribute.format = e.Format;
            attribute.location = e.Location;
            attribute.offset = e.Offset;
            VertexAttributes.push_back(attribute);
        }
        VertexInputState.vertexBindingDescriptionCount = (uint32_t)VertexBinding.size();
        VertexInputState.pVertexBindingDescriptions = VertexBinding.data();
        VertexInputState.vertexAttributeDescriptionCount = (uint32_t)VertexAttributes.size();
        VertexInputState.pVertexAttributeDescriptions = VertexAttributes.data();
    }
    createInfo.stageCount = 2;
    createInfo.pStages = Stages;
    createInfo.pVertexInputState = &VertexInputState;
    createInfo.pInputAssemblyState = &InputAssemblyState;
    createInfo.pTessellationState = &TessellationState;
    createInfo.pViewportState = &ViewportState;
    createInfo.pRasterizationState = &RasterizationState;
    createInfo.pMultisampleState = &MultisampleState;
    createInfo.pDepthStencilState = &DepthStencilState;
    createInfo.pColorBlendState = &ColorBlendState;
    createInfo.pDynamicState = &DynamicState;
    createInfo.layout = layout->GetLayout();
    createInfo.renderPass = framebuffer->GetRenderPass();
    createInfo.subpass = PassId;
    createInfo.basePipelineHandle = 0;
    createInfo.basePipelineIndex = 0;

    vkCreateGraphicsPipelines(_coreinterface->Device, nullptr, 1, &createInfo, 0, &Pipe);
}

void  egx::Pipeline::invalidate(const ref<PipelineLayout>& layout, const egxshader& compute)
{
    if (Pipe) vkDestroyPipeline(_coreinterface->Device, Pipe, nullptr);
    _graphics = false;
    VkComputePipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage.pNext = nullptr;
    createInfo.stage.flags = 0;
    createInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    createInfo.stage.module = compute.GetShader();
    createInfo.stage.pName = compute.GetEntryPoint().c_str();
    VkSpecializationInfo info = compute.GetSpecializationInfo();
    createInfo.stage.pSpecializationInfo = &info;
    createInfo.layout = layout->GetLayout();
    createInfo.basePipelineHandle = nullptr;
    createInfo.basePipelineIndex = 0;
    vkCreateComputePipelines(_coreinterface->Device, nullptr, 1, &createInfo, nullptr, &Pipe);
}
