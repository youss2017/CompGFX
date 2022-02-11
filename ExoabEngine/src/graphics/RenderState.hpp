#pragma once
#include "../core/memory/Buffers.hpp"
#include <vector>

/* Descripion of Vertex and Index Buffer */
class RenderState
{
public:
	static RenderState Create(IGPUBuffer VertexBuffer, uint32_t vertices_count);
	static RenderState Create(IGPUBuffer VertexBuffer, IGPUBuffer IndexBuffer, uint32_t vertices_count, uint32_t indices_count);

	void Destroy();

public:
	friend class RendererAPI;
	EngineAPIType m_ApiType;
	IGPUBuffer m_VertexBuffer;
	IGPUBuffer m_IndexBuffer;
	uint32_t m_VaoID = 0;
	bool m_UsingIndexBuffer = false;
	uint32_t m_VerticesCount, m_IndicesCount;
};