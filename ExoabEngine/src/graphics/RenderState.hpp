#pragma once
#include "../core/memory/Buffer2.hpp"
#include <vector>

/* Descripion of Vertex and Index Buffer */
class RenderState
{
public:
	static RenderState Create(IBuffer2 VertexBuffer, uint32_t vertices_count);
	static RenderState Create(IBuffer2 VertexBuffer, IBuffer2 IndexBuffer, uint32_t vertices_count, uint32_t indices_count);

	void Destroy();

public:
	friend class RendererAPI;
	EngineAPIType m_ApiType;
	IBuffer2 m_VertexBuffer;
	IBuffer2 m_IndexBuffer;
	uint32_t m_VaoID = 0;
	bool m_UsingIndexBuffer = false;
	uint32_t m_VerticesCount, m_IndicesCount;
};