#pragma once
#include "GLGraphicsCard.hpp"

struct Framebuffer;
typedef Framebuffer* IFramebuffer;

namespace gl
{

    class GraphicsSwapchain
    {
    public:
        static GraphicsSwapchain Create(gl::GLContext context, PlatformWindow *pWindow, bool UsingImGui);
        void SetSyncInterval(int sync_interval);
        void PrepareNextFrame();
        void Present(IFramebuffer framebuffer, int AttachmentIndex);
        void Destroy();

    private:
        gl::GLContext m_context;
        unsigned int m_ProgramID, m_DummyVAO;
        PlatformWindow *m_Window;
        bool m_UsingImGui;
    };

}