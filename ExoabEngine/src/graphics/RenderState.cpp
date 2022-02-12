#include "RenderState.hpp"
#include "../core/backend/GLGraphicsCard.hpp"

///////////////////////////////////// RENDER STATE
#if 0
static int GenerateGLStrideOffsetAndBindings(GLenum target, int StartVertexAttribArrayIndex, const GPUBufferSpecification &spec)
{
    if (target == GL_ARRAY_BUFFER)
    {
        // set opengl layout
        auto GetSize = [](EngineBufferElement e, int &size, GLenum &type, int &SizeInBytes) throw()->void
        {
            switch (e)
            {
            case EngineBufferElement::Float:
                SizeInBytes = 4;
                size = 1;
                type = GL_FLOAT;
                break;
            case EngineBufferElement::Float2:
                SizeInBytes = 8;
                size = 2;
                type = GL_FLOAT;
                break;
            case EngineBufferElement::Float3:
                SizeInBytes = 12;
                size = 3;
                type = GL_FLOAT;
                break;
            case EngineBufferElement::Float4:
                SizeInBytes = 16;
                size = 4;
                type = GL_FLOAT;
                break;
            case EngineBufferElement::Int:
                SizeInBytes = 4;
                size = 1;
                type = GL_INT;
                break;
            case EngineBufferElement::Int2:
                SizeInBytes = 8;
                size = 2;
                type = GL_INT;
                break;
            case EngineBufferElement::Int3:
                SizeInBytes = 12;
                size = 3;
                type = GL_INT;
                break;
            case EngineBufferElement::Int4:
                SizeInBytes = 16;
                size = 4;
                type = GL_INT;
                break;
            case EngineBufferElement::UInt:
                SizeInBytes = 4;
                size = 1;
                type = GL_UNSIGNED_INT;
                break;
            case EngineBufferElement::UInt2:
                SizeInBytes = 8;
                size = 2;
                type = GL_UNSIGNED_INT;
                break;
            case EngineBufferElement::UInt3:
                SizeInBytes = 12;
                size = 3;
                type = GL_UNSIGNED_INT;
                break;
            case EngineBufferElement::UInt4:
                SizeInBytes = 16;
                size = 4;
                type = GL_UNSIGNED_INT;
                break;
            default:
                assert(0);
                break;
            }
        };
        int stride = 0;
        if (spec.m_BufferType == BufferType::VertexBuffer)
        {
            for (int i = 0; i < spec.m_Elements.size(); i++)
            {
                int size = 0, SizeInBytes = 0;
                GLenum type;
                GetSize(spec.m_Elements[i], size, type, SizeInBytes);
                stride += SizeInBytes;
            }
            int offset = 0;
            for (int i = 0; i < spec.m_Elements.size(); i++)
            {
                int size, SizeInBytes;
                GLenum type;
                GetSize(spec.m_Elements[i], size, type, SizeInBytes);
                glVertexAttribPointer(i + StartVertexAttribArrayIndex, size, type, GL_FALSE, stride, (const void *)offset);
                glEnableVertexAttribArray(i + StartVertexAttribArrayIndex);
                offset += SizeInBytes;
            }
        }
    }
    return StartVertexAttribArrayIndex + spec.m_Elements.size();
}
#endif

RenderState RenderState::Create(IBuffer2 VertexBuffer, uint32_t vertices_count)
{
    RenderState state;
    void *context = VertexBuffer->m_context;
    if (*(char *)context == 0)
    {
        // vk
        state.m_VertexBuffer = VertexBuffer;
        state.m_ApiType = EngineAPIType::Vulkan;
    }
    else
    {
        // gl
        glGenVertexArrays(1, &state.m_VaoID);
        glBindVertexArray(state.m_VaoID);
        //auto spec = VertexBuffer.GetBufferSpecification();
        glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer->m_gl_buffer);
        //GenerateGLStrideOffsetAndBindings(GL_ARRAY_BUFFER, 0, spec);
        assert(0 && "Fix stride stuff!");
        glBindVertexArray(0);
        state.m_ApiType = EngineAPIType::OpenGL;
    }
    state.m_VerticesCount = vertices_count;
    state.m_IndicesCount = 0;
    return state;
}

RenderState RenderState::Create(IBuffer2 VertexBuffer, IBuffer2 IndexBuffer, uint32_t vertices_count, uint32_t indices_count)
{
    RenderState state;
    void* context = VertexBuffer->m_context;
    if (*(char *)context == 0)
    {
        state.m_ApiType = EngineAPIType::Vulkan;
        state.m_UsingIndexBuffer = true;
        state.m_VertexBuffer = VertexBuffer;
        state.m_IndexBuffer = IndexBuffer;
    }
    else
    {
        state.m_ApiType = EngineAPIType::OpenGL;
        glGenVertexArrays(1, &state.m_VaoID);
        glBindVertexArray(state.m_VaoID);
        //auto spec = VertexBuffer.GetBufferSpecification();
        glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer->m_gl_buffer);
        //GenerateGLStrideOffsetAndBindings(GL_ARRAY_BUFFER, 0, spec);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer->m_gl_buffer);
        glBindVertexArray(0);
    }
    state.m_VerticesCount = vertices_count;
    state.m_IndicesCount = indices_count;
    return state;
}

void RenderState::Destroy()
{
    if (m_ApiType == EngineAPIType::OpenGL)
    {
        glDeleteVertexArrays(1, &m_VaoID);
    }
}
