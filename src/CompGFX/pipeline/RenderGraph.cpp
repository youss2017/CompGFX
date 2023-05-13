#include "RenderGraph.hpp"

using namespace std;
using namespace egx;
using namespace cpp;

ResourceDescriptor::ResourceDescriptor(const DeviceCtx &pCtx, vk::DescriptorPool pool, const ShaderReflection &reflection, const std::map<uint32_t, vk::DescriptorSetLayout> &setLayouts)
{
    m_Data = make_shared<ResourceDescriptor::DataWrapper>();
    m_Data->m_Ctx = pCtx;
    m_Data->m_Pool = pool;
    m_Reflection = reflection;

    if (setLayouts.size() == 0)
    {
        throw std::runtime_error("Cannot create a resource descriptor with 0 descriptor set layouts.");
    }

    vector<vk::DescriptorSetLayout> layouts;
    for (auto &[_, setLayout] : setLayouts)
        layouts.push_back(setLayout);

    vk::DescriptorSetAllocateInfo allocateInfo;
    allocateInfo.descriptorPool = pool;
    allocateInfo.descriptorSetCount = (uint32_t)layouts.size();
    allocateInfo.pSetLayouts = layouts.data();
    auto sets = pCtx->Device.allocateDescriptorSets(allocateInfo);
    // Map vk::DescriptorSet with setId in std::map<uint32_t(setId), vk::DescriptorSet>
    {
        int i = 0;
        for (auto &[setId, _] : setLayouts)
        {
            m_Data->m_Sets[setId] = sets[i++];
        }
    }

    for (auto &[setId, bindings] : reflection.SetToManyBindings)
    {
        for (auto &[bindingId, info] : bindings)
        {
            if (info.IsDynamic && info.IsBuffer)
            {
                m_Data->m_Offsets.resize(m_Data->m_Offsets.size() + 1);
            }
        }
    }
}

ResourceDescriptor& egx::ResourceDescriptor::SetInput(int setId, int bindingId, const Buffer &buffer)
{
    vk::WriteDescriptorSet write;
    write.dstBinding = bindingId;
    write.dstSet = m_Data->m_Sets.at(setId);
    write.descriptorCount = 1;
    write.descriptorType = vk::DescriptorType(m_Reflection.SetToManyBindings.at(setId).at(bindingId).Type);
    const vk::DescriptorBufferInfo info = vk::DescriptorBufferInfo().setBuffer(buffer.GetHandle()).setRange(buffer.Size());
    write.setBufferInfo(info);
    m_Data->m_Ctx->Device.updateDescriptorSets(write, {});
    return *this;
}

ResourceDescriptor& egx::ResourceDescriptor::SetInput(int setId, int bindingId, vk::ImageLayout layout, int viewId, const Image2D &image, vk::Sampler sampler)
{
    vk::WriteDescriptorSet write;
    write.dstBinding = bindingId;
    write.dstSet = m_Data->m_Sets.at(setId);
    write.descriptorCount = 1;
    write.descriptorType = vk::DescriptorType(m_Reflection.SetToManyBindings.at(setId).at(bindingId).Type);
    const auto info = vk::DescriptorImageInfo()
        .setImageLayout(layout)
        .setImageView(image.GetView(viewId))
        .setSampler(sampler);
    write.setImageInfo(info);
    m_Data->m_Ctx->Device.updateDescriptorSets(write, {});
    return *this;
}

void egx::ResourceDescriptor::SetBufferOffset(int setId, int bindingId, uint32_t offset)
{
    m_Data->m_OffsetMapping[setId][bindingId] = offset;
}

void egx::ResourceDescriptor::Bind(vk::CommandBuffer cmd, vk::PipelineBindPoint bindPoint, vk::PipelineLayout layout)
{
    int offsetIndex = 0;
    for (auto &[setId, bindingOffsets] : m_Data->m_OffsetMapping)
    {
        for (auto &[bindingId, offset] : bindingOffsets)
        {
            m_Data->m_Offsets[offsetIndex] = offset;
            offsetIndex++;
        }
    }
    // The maximum descriptor sets on the best nvidia gpus is 8
    int setCount = 0;
    int firstSet = m_Data->m_Sets.begin()->first;
    array<vk::DescriptorSet, 16> sets;
    if(m_Data->m_Sets.size() > sets.size()) {
        throw logic_error("Too many sets");
    }
    for (auto &[setId, set] : m_Data->m_Sets)
    {
        sets[setCount++] = set;
    }
    cmd.bindDescriptorSets(bindPoint, layout, firstSet, setCount, sets.data(), (uint32_t)m_Data->m_Offsets.size(), m_Data->m_Offsets.data());
}

ResourceDescriptor::DataWrapper::~DataWrapper()
{
    LOG(WARNING, "(TODO) Free descriptor set.");
}

egx::RenderGraph::RenderGraph(const DeviceCtx &ctx)
{
    m_Data = make_shared<RenderGraph::DataWrapper>();
    m_Data->m_Ctx = ctx;
    for (auto i = 0u; i < ctx->FramesInFlight; i++)
    {
        auto cmdPool = ctx->Device.createCommandPool({});
        auto cmd = ctx->Device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(cmdPool).setCommandBufferCount(1));
        m_Data->m_Cmds.push_back({cmdPool, cmd[0], ctx->Device.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled))});
    }
    LOG(WARNING, "(TODO) Find a way to optimize descriptor pool.");
    vk::DescriptorPoolSize poolSizes[] = {
        {vk::DescriptorPoolSize().setType(vk::DescriptorType::eStorageBuffer).setDescriptorCount(1000)},
        {vk::DescriptorPoolSize().setType(vk::DescriptorType::eStorageBufferDynamic).setDescriptorCount(1000)},
        {vk::DescriptorPoolSize().setType(vk::DescriptorType::eSampledImage).setDescriptorCount(1000)},
        {vk::DescriptorPoolSize().setType(vk::DescriptorType::eStorageImage).setDescriptorCount(1000)},
        {vk::DescriptorPoolSize().setType(vk::DescriptorType::eSampler).setDescriptorCount(1000)},
        {vk::DescriptorPoolSize().setType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(1000)},
        {vk::DescriptorPoolSize().setType(vk::DescriptorType::eUniformBufferDynamic).setDescriptorCount(1000)},
    };
    m_Data->m_DescriptorPool = ctx->Device.createDescriptorPool(
        vk::DescriptorPoolCreateInfo()
            .setMaxSets(1000)
            .setPoolSizes(poolSizes)
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet | vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind));
}

RenderGraph& egx::RenderGraph::Add(uint32_t stageId, const PipelineType &pipeline, const function<void(vk::CommandBuffer)> &stageCallback, const GraphSynchronization& sync)
{
    if (m_Data->m_Stages.contains(stageId)) {
        throw runtime_error("Stage already exists in RenderGraph.");
    }
    m_Data->m_Stages[stageId] = Stage{stageId, pipeline.MakeHandle(), stageCallback, sync};
    return *this;
}

void RenderGraph::Run()
{
    const auto device = m_Data->m_Ctx->Device;
    const auto fence = RunAsync();
    device.waitForFences(fence, true, numeric_limits<uint64_t>::max());
}

RenderGraph &egx::RenderGraph::AddWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlags stageFlag)
{
    m_Data->m_WaitSemaphores.push_back(semaphore), m_Data->m_WaitStages.push_back(stageFlag);
    return *this;
}

RenderGraph &egx::RenderGraph::UseSignalSemaphore()
{
    m_Data->m_CompletionSemaphore = m_Data->m_Ctx->Device.createSemaphore(vk::SemaphoreCreateInfo());
    return *this;
}

vk::Fence egx::RenderGraph::RunAsync()
{
    const auto device = m_Data->m_Ctx->Device;
    const auto queue = m_Data->m_Ctx->Queue;
    const auto [cmdPool, cmd, fence] = m_Data->m_Cmds[m_Data->m_Ctx->CurrentFrame];
    device.waitForFences(fence, true, numeric_limits<uint64_t>::max());
    device.resetFences(fence);
    device.resetCommandPool(cmdPool);
    cmd.begin(vk::CommandBufferBeginInfo());
    for (auto &[id, stage] : m_Data->m_Stages)
    {
        cmd.bindPipeline(stage.Pipeline->BindPoint(), stage.Pipeline->Pipeline());
        stage.Callback(cmd);
        if(stage.Synchronization.BarrierCount()) {
            const auto& dependencyInfo = stage.Synchronization.ReadDependencyInfo();
            cmd.pipelineBarrier2(dependencyInfo);
        }
    }
    cmd.end();
    queue.submit(vk::SubmitInfo().setCommandBuffers(cmd), fence);
    return fence;
}

RenderGraph::DataWrapper::~DataWrapper()
{
    m_Ctx->Device.destroyDescriptorPool(m_DescriptorPool);
    for (auto &[pool, cmd, fence] : m_Cmds)
    {
        m_Ctx->Device.destroyFence(fence);
        m_Ctx->Device.destroyCommandPool(pool);
    }
}

ResourceDescriptor egx::RenderGraph::CreateResourceDescriptor(const PipelineType &pipeline)
{
    return ResourceDescriptor(m_Data->m_Ctx, m_Data->m_DescriptorPool, pipeline.Reflection(), pipeline.GetDescriptorSetLayouts());
}
