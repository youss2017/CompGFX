#include "DebugRenderer.hpp"
#include <backend/VkGraphicsCard.hpp>

DebugRenderer::DebugRenderer(GraphicsContext context)
{
	m_vertices_buffer = Buffer2_Create(context, BUFFER_TYPE_VERTEX, sizeof(DebugVertex) * 1000, BufferMemoryType::CPU_ONLY);
}

void DebugRenderer::Destroy()
{
}

void DebugRenderer::DrawPoint(float x, float y, float z)
{
}

void DebugRenderer::DrawLine(float x, float y, float z, float x2, float y2, float z2)
{
}

void DebugRenderer::DrawCircle(float x, float y, float z, float r)
{
}

void DebugRenderer::DrawBox(float x, float y, float z, float x2, float y2, float z2)
{
}

void DebugRenderer::DrawVertices(DebugVertex* vertices, unsigned int count)
{
}
