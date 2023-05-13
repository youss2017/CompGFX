#pragma once
#include <core/egx.hpp>
#include <memory/egximage.hpp>
#include <map>
#include <window/swapchain.hpp>

namespace egx
{

    class RenderTarget
    {
    public:
        struct Attachment {
            int32_t Id;
            Image2D Image;
            vk::AttachmentDescription2 Description;
            vk::ClearValue Clear;
            vk::ImageLayout SubpassLayout;
        };

    public:
        RenderTarget() = default;
        RenderTarget(const DeviceCtx &ctx, uint32_t width, uint32_t height);
        RenderTarget(
            const DeviceCtx& ctx, const SwapchainController& swapchain,
            vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
            vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, vk::ClearValue clearColor, vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare);

        virtual RenderTarget &CreateColorAttachment(int32_t id, vk::Format format,
                                            vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
                                            vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
                                            vk::ClearValue clearColor = {}, vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                            vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare);

        virtual RenderTarget &AddColorAttachment(int32_t id, const Image2D &image, vk::Format format,
                                         vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
                                         vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
                                         vk::ClearValue clearColor = {}, vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                         vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare);

        virtual RenderTarget &CreateSingleDepthAttachment(vk::Format format,
                                                  vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
                                                  vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, 
                                                  vk::ClearValue clearValue = {1.0f}, vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                                  vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare);

        virtual void Invalidate();
        virtual void Begin(vk::CommandBuffer cmd);
        virtual void End(vk::CommandBuffer cmd);

        virtual Image2D GetAttachment(int32_t id);
        virtual vk::AttachmentDescription2 GetAttachmentDescription(int32_t id);
        virtual bool ContainsAttachment(int32_t id) const { return m_Data->m_ColorAttachments.contains(id); }
        virtual const std::map<int32_t, Attachment>& EnumerateColorAttachments() const { return m_Data->m_ColorAttachments; }

        virtual uint32_t Width() const { return m_Data->m_Width; }
        virtual uint32_t Height() const { return m_Data->m_Height; }
        virtual vk::RenderPass RenderPass() const { return m_Data->m_RenderPass; }
        inline bool SwapchainFlag() const { return m_SwapchainFlag; }

    private:
        struct DataWrapper
        {
            DeviceCtx m_Ctx;
            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            std::map<int32_t, Attachment> m_ColorAttachments;
            std::optional<Attachment> m_DepthAttachment;
            vk::RenderPass m_RenderPass;
            vk::Framebuffer m_Framebuffer;
            std::vector<vk::Framebuffer> m_SwapchainFramebuffers;

            DataWrapper() = default;
            DataWrapper(DataWrapper&) = delete;
            ~DataWrapper();
        };
        bool m_SwapchainFlag = false;
        std::shared_ptr<DataWrapper> m_Data;
    };

} // namespace egx
