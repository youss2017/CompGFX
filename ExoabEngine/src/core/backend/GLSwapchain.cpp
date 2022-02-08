#include "GLSwapchain.hpp"
#include "../GUI.h"
#include "../pipeline/Framebuffer.hpp"
#include "../../utils/Logger.hpp"
#include <glad/glad.h>
#include <cassert>

static const char *s_VertexShaderCode =
    "#version 330 core\n"
    "#extension GL_ARB_shading_language_420pack : require\n"
    "const vec4 quad[] = \n"
    "{\n"
    "    // triangle 1\n"
    "    vec4(-1, 1, 0, 0),\n"
    "    vec4(1, -1, 1, 1),\n"
    "    vec4(-1, -1, 0, 1),\n"
    "    // triangle 2\n"
    "    vec4(-1, 1, 0, 0),\n"
    "    vec4(1, 1, 1, 0),\n"
    "    vec4(1, -1, 1, 1)\n"
    "};\n"
    "out vec2 TexCoord;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 Vertex = quad[gl_VertexID];\n"
    "    gl_Position = vec4(Vertex.xy, 0.0, 1.0);\n"
    "    TexCoord = Vertex.zw;	\n"
    "}\n";

static const char *s_FragmentShaderCode =
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D ColorPassTexture;\n"
    "out vec4 FragColor;\n"
    "// Do not use this function if the Output Texture is using an SRGBA format\n"
    "vec3 GammaCorrection(vec3 color, float gamma)\n"
    "{\n"
    "    return pow(color, vec3(1.0 / gamma));\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 Color = texture(ColorPassTexture, TexCoord);\n"
    "    //FragColor = vec4(GammaCorrection(Color.rgb, 2.2), Color.a);\n"
    "    FragColor = Color;\n"
    "}\n";

namespace gl
{

    GraphicsSwapchain GraphicsSwapchain::Create(gl::GLContext context, PlatformWindow *pWindow, bool UsingImGui)
    {
        assert(pWindow);
        GraphicsSwapchain gfxswap;
        gfxswap.m_context = context;
        gfxswap.m_Window = pWindow;
        gfxswap.m_UsingImGui = UsingImGui;
        printf("%s\n", (const char *)glGetString(GL_VERSION));

        GLuint vertexID = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(vertexID, 1, &s_VertexShaderCode, NULL);
        glShaderSource(fragmentID, 1, &s_FragmentShaderCode, NULL);
        glCompileShader(vertexID);
        glCompileShader(fragmentID);

        GLint vertex_compile_status, fragment_compile_status;
        glGetShaderiv(vertexID, GL_COMPILE_STATUS, &vertex_compile_status);
        glGetShaderiv(fragmentID, GL_COMPILE_STATUS, &fragment_compile_status);

        if (vertex_compile_status != GL_TRUE)
        {
            char log[256];
            glGetShaderInfoLog(vertexID, 256, NULL, log);
            strcat_s<256>(log, "\nGLSwapchain.cpp Vertex Shader");
            log_error(log, __FILE__, __LINE__);
            assert(0);
        }

        if (fragment_compile_status != GL_TRUE)
        {
            char log[256];
            glGetShaderInfoLog(fragmentID, 256, NULL, log);
            strcat_s<256>(log, "\nGLSwapchain.cpp Fragment Shader");
            log_error(log, __FILE__, __LINE__);
            assert(0);
        }

        gfxswap.m_ProgramID = glCreateProgram();
        glAttachShader(gfxswap.m_ProgramID, vertexID);
        glAttachShader(gfxswap.m_ProgramID, fragmentID);
        glLinkProgram(gfxswap.m_ProgramID);
        GLint program_link_status;
        glGetProgramiv(gfxswap.m_ProgramID, GL_LINK_STATUS, &program_link_status);

        if (program_link_status != GL_TRUE)
        {
            char log[256];
            glGetProgramInfoLog(gfxswap.m_ProgramID, 256, NULL, log);
            log_error(log, __FILE__, __LINE__);
            assert(0);
        }
        glDeleteShader(vertexID);
        glDeleteShader(fragmentID);

        if (UsingImGui)
        {
            Gui_GLInitalizeImGui(pWindow);
        }

        glGenVertexArrays(1, &gfxswap.m_DummyVAO);
        GLuint location = glGetUniformLocation(gfxswap.m_ProgramID, "ColorPassTexture");
        glUseProgram(gfxswap.m_ProgramID);
        glBindVertexArray(gfxswap.m_DummyVAO);
        glUniform1i(location, 0);
        glBindVertexArray(0);
        glUseProgram(0);
        return gfxswap;
    }

    void GraphicsSwapchain::SetSyncInterval(int sync_interval)
    {
        gl::Gfx_SetVSyncState(sync_interval);
    }

    void GraphicsSwapchain::PrepareNextFrame()
    {
        if (m_UsingImGui)
            Gui_GLBeginGUIFrame();
    }

    void GraphicsSwapchain::Present(IFramebuffer _framebuffer, int AttachmentIndex)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        int w = m_Window->GetWidth();
        int h = m_Window->GetHeight();
        glViewport(0, 0, w, h);
        glScissor(0, 0, w, h);
        // restore pipeline state
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(m_ProgramID);
        glBindVertexArray(m_DummyVAO);
        glUniform1i(glGetUniformLocation(m_ProgramID, "ColorPassTexture"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, PVOIDToUInt32(Framebuffer_GetImage(_framebuffer, AttachmentIndex)));
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glUseProgram(0);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        if (m_UsingImGui)
            Gui_GLEndGUIFrame();
        gl::Gfx_Present(m_context);
    }

    void GraphicsSwapchain::Destroy()
    {
        glDeleteVertexArrays(1, &m_DummyVAO);
        glDeleteProgram(m_ProgramID);
        Gui_GLDestroyImGui();
    }

}