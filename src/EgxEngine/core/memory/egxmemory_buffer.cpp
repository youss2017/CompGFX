#include "egxmemory.hpp"
#include "../egxutil.hpp"
#include "stb/stb_image.h"
#include "stb/stb_image_resize.h"
#include "../../cmd/cmd.hpp"
#include <imgui/backends/imgui_impl_vulkan.h>
#include <cmath>
#include <vector>
#include <Utility/CppUtility.hpp>

using namespace egx;

egx::ref<egx::Buffer> egx::Buffer::FactoryCreate(
    const ref<VulkanCoreInterface> &CoreInterface,
    size_t size,
    memorylayout layout,
    uint32_t type,
    bool CpuWritePerFrameFlag,
    bool requireCoherent)
{
#ifdef _DEBUG
    if (size > 1 * (1024 * 1024))
    {
        LOG(INFO, "Creating a {0:%.2lf} Mb Buffer", (double)size / (1024.0 * 1024.0));
    }
#endif
    VkAlloc::DEVICE_MEMORY_PROPERTY property;
    VkFlags bufferUsage =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    switch (layout)
    {
    case (memorylayout::local):
    {
        property = VkAlloc::DEVICE_MEMORY_PROPERTY::GPU_ONLY;
        break;
    }
    case (memorylayout::dynamic):
    {
        property = VkAlloc::DEVICE_MEMORY_PROPERTY::CPU_TO_GPU;
        break;
    }
    case (memorylayout::stream):
    {
        property = VkAlloc::DEVICE_MEMORY_PROPERTY::CPU_ONLY;
        break;
    }
    default:
        property = VkAlloc::DEVICE_MEMORY_PROPERTY::CPU_ONLY;
    }
    if (type & BufferType_Vertex)
        bufferUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (type & BufferType_Index)
        bufferUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (type & BufferType_Uniform)
        bufferUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (type & BufferType_Storage)
        bufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (type & BufferType_Indirect)
        bufferUsage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    VkAlloc::BUFFER_DESCRIPTION desc{};
    desc.m_properties = property;
    desc.m_usage = bufferUsage;
    desc.m_size = size;
    desc.mRequireCoherent = requireCoherent;
    auto result = new egx::Buffer(
        size, layout,
        type, CpuWritePerFrameFlag,
        requireCoherent, CoreInterface->MemoryContext,
        CoreInterface);
    for (size_t i = 0; i < (CpuWritePerFrameFlag ? CoreInterface->MaxFramesInFlight : 1); i++)
    {
        VkAlloc::BUFFER buffer;
        VkResult ret = VkAlloc::CreateBuffers(CoreInterface->MemoryContext, 1, &desc, &buffer);
        if (ret != VK_SUCCESS)
        {
            for (auto &buf : result->_buffers)
            {
                VkAlloc::DestroyBuffers(CoreInterface->MemoryContext, 1, &buf);
            }
            delete result;
            return nullptr;
        }
        result->_buffers.push_back(buffer);
    }
    return result;
}

egx::Buffer::~Buffer()
{
    for (auto &buf : _buffers)
    {
        VkAlloc::DestroyBuffers(_coreinterface->MemoryContext, 1, &buf);
    }
}

egx::Buffer::Buffer(Buffer &&move) noexcept
    : Size(move.Size), Layout(move.Layout),
      Type(move.Type), CoherentFlag(move.CoherentFlag),
      CpuAccessPerFrame(move.CpuAccessPerFrame), _context(move._context)
{
    memcpy(this, &move, sizeof(Buffer));
    memset(&move, 0, sizeof(Buffer));
}

egx::Buffer &egx::Buffer::operator=(Buffer &move) noexcept
{
    memcpy(this, &move, sizeof(Buffer));
    memset(&move, 0, sizeof(Buffer));
    return *this;
}

egx::ref<egx::Buffer> egx::Buffer::Clone()
{
    auto clone = Buffer::FactoryCreate(_coreinterface, Size, Layout, Type, CpuAccessPerFrame, CoherentFlag);
    if (!clone.IsValidRef())
        return clone;
    clone->Copy(this);
    return clone;
}

void egx::Buffer::Copy(ref<Buffer> &source)
{
    Copy(source.base);
}

void egx::Buffer::Copy(Buffer *source)
{
    assert(Size == source->Size);
    Copy(source->_buffers[GetCurrentFrame()]->m_buffer, 0, Size);
}

void egx::Buffer::Write(void *data, size_t offset, size_t size)
{
    if (Layout != memorylayout::local)
    {
        bool mapState = _ptr == nullptr;
        char *ptr = (char*)Map();
        ptr += offset;
        memcpy(ptr, data, size);
        if (mapState)
            Unmap();
        return;
    }
    ref<Buffer> stage = FactoryCreate(_coreinterface, size, memorylayout::stream, Type, false);
    stage->Write(data);
    stage->Flush();
    Copy(stage->_buffers[0]->m_buffer, offset, size);
}

void egx::Buffer::Write(void *data, size_t size)
{
    return Write(data, 0, size);
}

void egx::Buffer::Write(void *data)
{
    return Write(data, 0, Size);
}

void *egx::Buffer::Map()
{
    assert(Layout != memorylayout::local);
    if (_ptr)
        return _ptr;
    VkAlloc::MapBuffer(_context, _buffers[GetCurrentFrame()]);
    _ptr = _buffers[GetCurrentFrame()]->m_suballocation.m_allocation_info.pMappedData;
    return _ptr;
}

void egx::Buffer::Unmap()
{
    assert(Layout != memorylayout::local);
    _ptr = nullptr;
    VkAlloc::UnmapBuffer(_context, _buffers[GetCurrentFrame()]);
}

void egx::Buffer::Read(void *pOutput, size_t offset, size_t size)
{
    assert(offset + size < Size);
    if (Layout != memorylayout::local)
    {
        bool mapState = _ptr == nullptr;
        char *ptr = (char *)(this->Map());
        ptr += offset;
        memcpy(pOutput, ptr, size);
        return;
    }
    ref<Buffer> stage = FactoryCreate(_coreinterface, size, memorylayout::stream, BufferType_TransferOnly, false);
    auto cmd = CommandBufferSingleUse(_coreinterface);

    {
        VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        barrier.buffer = GetBuffer();
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.srcQueueFamilyIndex =
            barrier.dstQueueFamilyIndex =
                VK_QUEUE_FAMILY_IGNORED;
        barrier.offset = offset;
        barrier.size = size;
        vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                             1, &barrier,
                             0, nullptr);
    }

    {
        VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
        barrier.buffer = stage->GetBuffer();
        barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.srcQueueFamilyIndex =
            barrier.dstQueueFamilyIndex =
                VK_QUEUE_FAMILY_IGNORED;
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;
        vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                             1, &barrier,
                             0, nullptr);
    }

    VkBufferMemoryBarrier barriers[2]{};
    barriers[0].sType = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barriers[0].buffer = GetBuffer();
    barriers[0].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barriers[0].srcQueueFamilyIndex =
        barriers[0].dstQueueFamilyIndex =
            VK_QUEUE_FAMILY_IGNORED;
    barriers[0].offset = offset;
    barriers[0].size = size;

    barriers[1].buffer = stage->GetBuffer();
    barriers[1].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    barriers[1].srcQueueFamilyIndex =
        barriers[1].dstQueueFamilyIndex =
            VK_QUEUE_FAMILY_IGNORED;
    barriers[1].offset = 0;
    barriers[1].size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                         2, barriers,
                         0, nullptr);

    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = size;
    vkCmdCopyBuffer(cmd.Cmd, GetBuffer(), stage->GetBuffer(), 1, &region);
    cmd.Execute();

    stage->Invalidate();
    char *ptr = (char*)(stage->Map());
    ptr += offset;
    memcpy(pOutput, ptr, size);
}

void egx::Buffer::Read(void *pOutput, size_t size)
{
    this->Read(pOutput, 0, size);
}

void egx::Buffer::Read(void *pOutput)
{
    this->Read(pOutput, 0, Size);
}

void egx::Buffer::Flush()
{
    VkAlloc::FlushBuffer(_context, _buffers[GetCurrentFrame()], 0, Size);
}

void egx::Buffer::Flush(size_t offset, size_t size)
{
    assert(offset + size < Size);
    VkAlloc::FlushBuffer(_context, _buffers[GetCurrentFrame()], offset, size);
}

void egx::Buffer::Invalidate()
{
    VkAlloc::InvalidateBuffer(_context, _buffers[GetCurrentFrame()], 0, Size);
}

void egx::Buffer::Invalidate(size_t offset, size_t size)
{
    assert(offset + size < Size);
    VkAlloc::InvalidateBuffer(_context, _buffers[GetCurrentFrame()], offset, size);
}

void egx::Buffer::Copy(VkBuffer src, size_t offset, size_t size)
{
    auto cmd = CommandBufferSingleUse(_coreinterface);

    VkBufferMemoryBarrier barriers[2]{};
    barriers[0].sType = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barriers[0].buffer = GetBuffer();
    barriers[0].dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    barriers[0].srcQueueFamilyIndex =
        barriers[0].dstQueueFamilyIndex =
            VK_QUEUE_FAMILY_IGNORED;
    barriers[0].offset = offset;
    barriers[0].size = size;

    barriers[1].sType = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barriers[1].buffer = src;
    barriers[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barriers[1].srcQueueFamilyIndex =
        barriers[1].dstQueueFamilyIndex =
            VK_QUEUE_FAMILY_IGNORED;
    barriers[1].offset = 0;
    barriers[1].size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(cmd.Cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                         2, barriers,
                         0, nullptr);

    VkBufferCopy region{};
    region.srcOffset = offset;
    region.size = size;
    vkCmdCopyBuffer(cmd.Cmd, src, GetBuffer(), 1, &region);

    cmd.Execute();
}

const VkBuffer& Buffer::GetBuffer() const {
    return _buffers[GetCurrentFrame()]->m_buffer;
}
