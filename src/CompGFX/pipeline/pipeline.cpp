#include "pipeline.hpp"
#include <vector>
#include "pipeline.hpp"

using namespace std;
using namespace egx;
using namespace vk;

egx::ComputePipeline::ComputePipeline(const DeviceCtx &pCtx, const Shader &computeShader)
{
    m_Data = make_shared<ComputePipeline::DataWrapper>();
    m_Data->m_Ctx = pCtx;
    m_Data->m_SetLayouts = Shader::CreateDescriptorSetLayouts({computeShader});
    m_Data->m_Reflection = computeShader.Reflection();
    auto pushblock = Shader::GetPushconstants({computeShader});
    const vk::SpecializationInfo specialConstants = vk::SpecializationInfo(computeShader.GetSpecializationConstants());
    auto scalarSetLayouts = Shader::GetDescriptorSetLayoutsAsScalar(m_Data->m_SetLayouts);

    vk::PipelineLayoutCreateInfo createInfo;
    createInfo.setSetLayouts(scalarSetLayouts).setPushConstantRanges(pushblock);
    m_Data->m_Layout = pCtx->Device.createPipelineLayout(createInfo);

    vk::ComputePipelineCreateInfo computeCreateInfo;
    computeCreateInfo.stage.setStage(vk::ShaderStageFlagBits::eCompute)
        .setModule(computeShader.GetModule())
        .setPSpecializationInfo(&specialConstants)
        .setPName("main");
    computeCreateInfo.setLayout(m_Data->m_Layout);
    m_Data->m_Pipeline = pCtx->Device.createComputePipeline(nullptr, computeCreateInfo).value;
}

egx::ComputePipeline::DataWrapper::~DataWrapper()
{
    if (m_Ctx && m_Pipeline)
    {
        for (auto &[id, setLayout] : m_SetLayouts)
            m_Ctx->Device.destroyDescriptorSetLayout(setLayout);
        m_Ctx->Device.destroyPipelineLayout(m_Layout);
        m_Ctx->Device.destroyPipeline(m_Pipeline);
    }
}

egx::IGraphicsPipeline::IGraphicsPipeline(const DeviceCtx &pCtx, const Shader &vertex, const Shader &fragment, const IRenderTarget &rt, const PipelineSpecification &specification)
    : Specification(specification)
{
    m_Data = make_shared<IGraphicsPipeline::DataWrapper>();
    m_Data->m_Ctx = pCtx;
    m_Data->m_Vertex = vertex;
    m_Data->m_Fragment = fragment;
    m_Data->m_Reflection = ShaderReflection::Combine({vertex.Reflection(), fragment.Reflection()});
    m_Data->m_RenderTarget = rt;
    if (rt.SwapchainFlag()) {
        m_Data->m_BlendStates[-1] = DefaultBlendingPreset();
    }
    for (auto &[id, attachment] : rt.EnumerateColorAttachments())
    {
        if (id < 0 && m_Data->m_RenderTarget.SwapchainFlag())
            continue;
        m_Data->m_BlendStates[id] = DefaultBlendingPreset();
    }
    rt.GetSwapchain().AddResizeCallback(reinterpret_cast<IUniqueWithCallback*>(this), nullptr);
}

void egx::IGraphicsPipeline::Invalidate()
{
    m_Data->Reinvalidate();

    // Create descriptor set layout
    m_Data->m_SetLayouts = Shader::CreateDescriptorSetLayouts({ m_Data->m_Vertex,m_Data->m_Fragment});
    auto scalarSetLayouts = Shader::GetDescriptorSetLayoutsAsScalar(m_Data->m_SetLayouts);
    auto pushblock = Shader::GetPushconstants({ m_Data->m_Vertex, m_Data->m_Fragment});
    // Create pipeline layout
    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setSetLayouts(scalarSetLayouts)
        .setPushConstantRanges(pushblock);
    m_Data->m_Layout = m_Data->m_Ctx->Device.createPipelineLayout(layoutCreateInfo);

    GraphicsPipelineCreateInfo createInfo;

    // 1) Shader Stages
    array<PipelineShaderStageCreateInfo, 2> stages;
    stages[0].setModule(m_Data->m_Vertex.GetModule()).setPName("main").setStage(vk::ShaderStageFlagBits::eVertex).setPSpecializationInfo(&m_Data->m_Vertex.GetSpecializationConstants());
    stages[1].setModule(m_Data->m_Fragment.GetModule()).setPName("main").setStage(vk::ShaderStageFlagBits::eFragment).setPSpecializationInfo(&m_Data->m_Fragment.GetSpecializationConstants());

    // 2) Vertex Shader I/O
    auto vsReflection = m_Data->m_Vertex.Reflection();
    PipelineVertexInputStateCreateInfo vertexInput;

    vector<VertexInputBindingDescription> vertexBindings;
    vector<VertexInputAttributeDescription> vertexAttributes;
    const auto &vertexShaderInputs = vsReflection.IOBindingToManyLocationIn;
    for (auto &[bindingId, input] : vertexShaderInputs)
    {
        uint32_t offset = 0;
        for (auto &[locationId, io] : input)
        {
            VertexInputAttributeDescription attribute{};
            attribute.binding = io.Binding;
            attribute.format = Format(io.Format);
            attribute.location = io.Location;
            attribute.offset = offset;
            offset += io.Size;
            vertexAttributes.push_back(attribute);
        }
        VertexInputBindingDescription inputBinding{};
        inputBinding.binding = bindingId;
        inputBinding.inputRate = VertexInputRate::eVertex;
        inputBinding.stride = offset;
        vertexBindings.push_back(inputBinding);
    }
    vertexInput.setVertexBindingDescriptions(vertexBindings)
        .setVertexAttributeDescriptions(vertexAttributes);

    PipelineInputAssemblyStateCreateInfo InputAssemblyState{};
    InputAssemblyState.topology = Specification.Topology;
    InputAssemblyState.primitiveRestartEnable = VK_FALSE;

    PipelineTessellationStateCreateInfo TessellationState{};
    TessellationState.patchControlPoints = 1;

    // The viewport and scissor are based on the dimensions of the framebuffer.
    Viewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = float(Specification.ViewportWidth == 0 ? m_Data->m_RenderTarget.Width() : Specification.ViewportWidth);
    viewport.height = float(Specification.ViewportHeight == 0 ? m_Data->m_RenderTarget.Height() : Specification.ViewportHeight);
    viewport.minDepth = Specification.NearField;
    viewport.maxDepth = Specification.FarField;

    Rect2D scissor{};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = uint32_t(viewport.width);
    scissor.extent.height = uint32_t(viewport.height);

    PipelineViewportStateCreateInfo ViewportState{};
    ViewportState.viewportCount = 1;
    ViewportState.pViewports = &viewport;
    ViewportState.scissorCount = 1;
    ViewportState.pScissors = &scissor;

    PipelineRasterizationStateCreateInfo RasterizationState{};
    RasterizationState.depthClampEnable = VK_FALSE;
    RasterizationState.rasterizerDiscardEnable = VK_FALSE;
    RasterizationState.polygonMode = Specification.FillMode;
    RasterizationState.cullMode = Specification.CullMode;
    RasterizationState.frontFace = Specification.FrontFace;
    RasterizationState.depthBiasEnable = VK_FALSE;
    RasterizationState.depthBiasConstantFactor = 0.0f;
    RasterizationState.depthBiasClamp = 0.0f;
    RasterizationState.depthBiasSlopeFactor = 0.0f;
    RasterizationState.lineWidth = Specification.LineWidth;

    PipelineMultisampleStateCreateInfo MultisampleState{};
    MultisampleState.rasterizationSamples = SampleCountFlagBits::e1;
    MultisampleState.sampleShadingEnable = VK_FALSE;
    MultisampleState.minSampleShading = 0.0f;
    MultisampleState.pSampleMask = nullptr;
    MultisampleState.alphaToCoverageEnable = VK_FALSE;
    MultisampleState.alphaToOneEnable = VK_FALSE;
    LOG(WARNING, "(TODO) Implement Multisamples in Pipeline");

    PipelineDepthStencilStateCreateInfo DepthStencilState{};
    DepthStencilState.depthTestEnable = Specification.DepthEnabled;
    DepthStencilState.depthWriteEnable = Specification.DepthWriteEnable;
    DepthStencilState.depthCompareOp = Specification.DepthCompare;
    DepthStencilState.depthBoundsTestEnable = VK_FALSE;
    DepthStencilState.stencilTestEnable = VK_FALSE;
    DepthStencilState.minDepthBounds = Specification.NearField;
    DepthStencilState.maxDepthBounds = Specification.FarField;

    vector<PipelineColorBlendAttachmentState> BlendAttachmentStates;
    for (auto &[id, state] : m_Data->m_BlendStates)
    {
        BlendAttachmentStates.push_back(state);
    }

    PipelineColorBlendStateCreateInfo ColorBlendState;
    ColorBlendState.setAttachments(BlendAttachmentStates)
        .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f})
        .setLogicOpEnable(false);

    vector<DynamicState> states;
    if (Specification.DynamicViewport)
    {
        states.push_back(DynamicState::eViewport);
    }
    if (Specification.DynamicScissor)
    {
        states.push_back(DynamicState::eScissor);
    }

    PipelineDynamicStateCreateInfo DynamicState;
    DynamicState.setDynamicStates(states);

    createInfo
        .setStages(stages)
        .setPVertexInputState(&vertexInput)
        .setPInputAssemblyState(&InputAssemblyState)
        .setPTessellationState(&TessellationState)
        .setPViewportState(&ViewportState)
        .setPRasterizationState(&RasterizationState)
        .setPMultisampleState(&MultisampleState)
        .setPDepthStencilState(&DepthStencilState)
        .setPColorBlendState(&ColorBlendState)
        .setPDynamicState(&DynamicState)
        .setLayout(m_Data->m_Layout)
        .setRenderPass(m_Data->m_RenderTarget.RenderPass())
        .setSubpass(0);
    auto result = m_Data->m_Ctx->Device.createGraphicsPipeline(nullptr, createInfo);
    if (result.result != vk::Result::eSuccess)
    {
        throw runtime_error(cpp::Format("Could not create pipeline, vk::Result = {}", vk::to_string(result.result)));
    }
    m_Data->m_Pipeline = result.value;
}

void egx::IGraphicsPipeline::CallbackProtocol(void* pUserData)
{
    Invalidate();
}

void egx::IGraphicsPipeline::DataWrapper::Reinvalidate()
{
    for (auto &[id, setLayout] : m_SetLayouts)
        m_Ctx->Device.destroyDescriptorSetLayout(setLayout);
    m_Ctx->Device.destroyPipelineLayout(m_Layout);
    m_Ctx->Device.destroyPipeline(m_Pipeline);
    m_Pipeline = nullptr, m_Layout = nullptr;
    m_SetLayouts.clear();
}

egx::IGraphicsPipeline::DataWrapper::~DataWrapper()
{
    if (m_Ctx && m_Pipeline)
    {
        Reinvalidate();
    }
}