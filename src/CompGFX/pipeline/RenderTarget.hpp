#pragma once
#include <core/egx.hpp>
#include <memory/egximage.hpp>
#include <window/swapchain.hpp>
#include "DearImGuiController.hpp"
#include <map>

namespace egx
{

    class IRenderTarget : public IUniqueWithCallback
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
        IRenderTarget() = default;
        IRenderTarget(const DeviceCtx &ctx, uint32_t width, uint32_t height);
        IRenderTarget(
            const DeviceCtx& ctx, const ISwapchainController& swapchain,
            vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
            vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, vk::ClearValue clearColor, vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare);

        virtual IRenderTarget &CreateColorAttachment(int32_t id, vk::Format format,
                                            vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
                                            vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
                                            vk::ClearValue clearColor = {}, vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                            vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare);

        virtual IRenderTarget &AddColorAttachment(int32_t id, const Image2D &image, vk::Format format,
                                         vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
                                         vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp,
                                         vk::ClearValue clearColor = {}, vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                         vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare);

        virtual IRenderTarget &CreateSingleDepthAttachment(vk::Format format,
                                                  vk::ImageLayout initialLayout, vk::ImageLayout subpassLayout, vk::ImageLayout finalLayout,
                                                  vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp, 
                                                  vk::ClearDepthStencilValue clearValue = {1.0f}, vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                                  vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eDontCare);

        virtual IRenderTarget& EnableDearImGui(const PlatformWindow& window);
        virtual ImGuiContext* GetImGuiContext();

        virtual void Invalidate();
        virtual void Begin(vk::CommandBuffer cmd);
        virtual void End(vk::CommandBuffer cmd);

        virtual void BeginDearImguiFrame();
        /// <summary>
        /// This function must be called before End() because
        /// this function issues the draw calls for imgui and it must
        /// be within the render pass.
        /// </summary>
        /// <param name="cmd"></param>
        virtual void EndDearImguiFrame(vk::CommandBuffer cmd);

        virtual Image2D GetAttachment(int32_t id);
        virtual vk::AttachmentDescription2 GetAttachmentDescription(int32_t id);
        virtual bool ContainsAttachment(int32_t id) const { return m_Data->m_ColorAttachments.contains(id); }
        virtual const std::map<int32_t, Attachment>& EnumerateColorAttachments() const { return m_Data->m_ColorAttachments; }

        virtual uint32_t Width() const { return m_Data->m_Width; }
        virtual uint32_t Height() const { return m_Data->m_Height; }
        virtual vk::RenderPass RenderPass() const { return m_Data->m_RenderPass; }
        inline bool SwapchainFlag() const { return m_Data->m_SwapchainFlag; }
        inline ISwapchainController GetSwapchain() const { return m_Data->m_Swapchain; }

    public:
        virtual std::unique_ptr<IUniqueHandle> MakeHandle() const override;
        virtual void CallbackProtocol(void* pUserData) override;

    private:
        void FetchSwapchainBackBuffers();

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
            DearImGuiController m_ImGuiController;

            bool m_SwapchainFlag = false;
            bool m_DearImGuiFlag = false;
            GLFWwindow* m_WindowPtr = nullptr;
            ISwapchainController m_Swapchain;

            struct {
                vk::ImageLayout initialLayout;
                vk::ImageLayout subpassLayout;
                vk::ImageLayout finalLayout;
                vk::AttachmentLoadOp loadOp;
                vk::AttachmentStoreOp storeOp;
                vk::ClearValue clearColor;
                vk::AttachmentLoadOp stencilLoadOp;
                vk::AttachmentStoreOp stencilStoreOp;
            } swapchain_info;

            void Reinvalidate();

            DataWrapper() = default;
            DataWrapper(DataWrapper&) = delete;
            ~DataWrapper();
        };
      
        std::shared_ptr<DataWrapper> m_Data;
    };

} // namespace egx
