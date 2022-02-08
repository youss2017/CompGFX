#pragma once

#include "Model.hpp"
#include <glad/glad.h>

template <class VertexType>
OmegaModel<VertexType>::OmegaModel(GraphicsContext context, const char *mesh_path)
{
	if (sizeof(VertexType) == sizeof(OmegaBasicVertex))
	{
		m_basic = Omega_LoadBasicMesh(mesh_path);
		for (size_t i = 0; i < m_basic.m_submesh.size(); i++)
		{
			auto &submesh = m_basic.m_submesh[i];
			auto &vertices = submesh.m_vertices;
			auto &indices = submesh.m_indices;
			GPUBufferSpecification vertex_specification;
			vertex_specification.Initalize(BufferType::VertexBuffer, vertices.size() * sizeof(OmegaBasicVertex), BufferMemoryType::STATIC);
			GPUBufferSpecification index_specification;
			index_specification.Initalize(BufferType::IndexBuffer, indices.size() * sizeof(uint32_t), BufferMemoryType::STATIC);
			IGPUBuffer vertex_buffer = GPUBuffer_Create(context, &vertex_specification);
			IGPUBuffer index_buffer = GPUBuffer_Create(context, &index_specification);
			GPUBuffer_UploadData(vertex_buffer, vertices.data(), 0, vertices.size() * sizeof(OmegaBasicVertex));
			GPUBuffer_UploadData(index_buffer, indices.data(), 0, indices.size() * sizeof(uint32_t));
			m_buffers.push_back(vertex_buffer);
			m_buffers.push_back(index_buffer);
			RenderState state = RenderState::Create(vertex_buffer, index_buffer, vertices.size(), indices.size());
			if (*(char *)context == 1)
			{
				glBindVertexArray(state.m_VaoID);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaBasicVertex), ((void *)(0)));
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaBasicVertex), ((void *)(sizeof(glm::fvec3))));
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(OmegaBasicVertex), ((void *)(sizeof(glm::fvec3) * 2)));
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glEnableVertexAttribArray(2);
				glBindVertexArray(0);
			}
			m_state.push_back(state);
		}
	}
	else if (sizeof(VertexType) == sizeof(OmegaVertex))
	{
		m_typical = Omega_LoadMesh(mesh_path);
		for (size_t i = 0; i < m_typical.m_submesh.size(); i++)
		{
			auto &submesh = m_typical.m_submesh[i];
			auto &vertices = submesh.m_vertices;
			auto &indices = submesh.m_vertices;
			GPUBufferSpecification vertex_specification;
			vertex_specification.Initalize(BufferType::VertexBuffer, vertices.size() * sizeof(OmegaVertex), BufferMemoryType::STATIC);
			GPUBufferSpecification index_specification;
			index_specification.Initalize(BufferType::IndexBuffer, indices.size() * sizeof(uint32_t), BufferMemoryType::STATIC);
			IGPUBuffer vertex_buffer = GPUBuffer_Create(context, &vertex_specification);
			IGPUBuffer index_buffer = GPUBuffer_Create(context, &index_specification);
			GPUBuffer_UploadData(vertex_buffer, vertices.data(), 0, vertices.size() * sizeof(OmegaBasicVertex));
			GPUBuffer_UploadData(index_buffer, indices.data(), 0, indices.size() * sizeof(uint32_t));
			m_buffers.push_back(vertex_buffer);
			m_buffers.push_back(index_buffer);
			RenderState state = RenderState::Create(vertex_buffer, index_buffer, vertices.size(), indices.size());
			if (*(char *)context == 1)
			{
				glBindVertexArray(state.m_VaoID);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaVertex), ((void *)(0)));
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaVertex), ((void *)(sizeof(glm::fvec3))));
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaVertex), ((void *)(sizeof(glm::fvec3) * 2)));
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaVertex), ((void *)(sizeof(glm::fvec3) * 3)));
				glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(OmegaVertex), ((void *)(sizeof(glm::fvec3) * 4)));
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glEnableVertexAttribArray(2);
				glEnableVertexAttribArray(3);
				glEnableVertexAttribArray(4);
				glBindVertexArray(0);
			}
			m_state.push_back(state);
		}
	}
	else if (sizeof(VertexType) == sizeof(OmegaAnimatedVertex))
	{
		m_animated = Omega_LoadAnimatedMesh(mesh_path);
		for (size_t i = 0; i < m_animated.m_submesh.size(); i++)
		{
			auto &submesh = m_animated.m_submesh[i];
			auto &vertices = submesh.m_vertices;
			auto &indices = submesh.m_vertices;
			GPUBufferSpecification vertex_specification;
			vertex_specification.Initalize(BufferType::VertexBuffer, vertices.size() * sizeof(OmegaAnimatedVertex), BufferMemoryType::STATIC);
			GPUBufferSpecification index_specification;
			index_specification.Initalize(BufferType::IndexBuffer, indices.size() * sizeof(uint32_t), BufferMemoryType::STATIC);
			IGPUBuffer vertex_buffer = GPUBuffer_Create(context, &vertex_specification);
			IGPUBuffer index_buffer = GPUBuffer_Create(context, &index_specification);
			GPUBuffer_UploadData(vertex_buffer, vertices.data(), 0, vertices.size() * sizeof(OmegaBasicVertex));
			GPUBuffer_UploadData(index_buffer, indices.data(), 0, indices.size() * sizeof(uint32_t));
			m_buffers.push_back(vertex_buffer);
			m_buffers.push_back(index_buffer);
			RenderState state = RenderState::Create(vertex_buffer, index_buffer, vertices.size(), indices.size());
			if (*(char *)context == 1)
			{

				glBindVertexArray(state.m_VaoID);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaAnimatedVertex), ((void *)(0)));
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaAnimatedVertex), ((void *)(sizeof(glm::fvec3))));
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaAnimatedVertex), ((void *)(sizeof(glm::fvec3) * 2)));
				glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(OmegaAnimatedVertex), ((void *)(sizeof(glm::fvec3) * 3)));
				glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(OmegaAnimatedVertex), ((void *)(sizeof(glm::fvec3) * 4)));
				glVertexAttribIPointer(5, 4, GL_INT, sizeof(OmegaAnimatedVertex), ((void *)(sizeof(float) * 12)));
				glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, sizeof(OmegaAnimatedVertex), ((void *)(sizeof(float) * 16 + sizeof(int) * 4)));
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glEnableVertexAttribArray(2);
				glEnableVertexAttribArray(3);
				glEnableVertexAttribArray(4);
				glEnableVertexAttribArray(5);
				glEnableVertexAttribArray(6);
				glBindVertexArray(0);
			}
			m_state.push_back(state);
		}
	}
	else
	{
		assert(0 && "Invalid VertexType struct valid ones are OmegaBasicVertex, OmegaVertex, OmegaAnimatedVertex");
	}
}

template <class VertexType>
OmegaModel<VertexType>::~OmegaModel()
{
	for (size_t i = 0; i < m_state.size(); i++)
		m_state[i].Destroy();
	for (size_t i = 0; i < m_buffers.size(); i++)
		GPUBuffer_Destroy(m_buffers[i]);
}

template <class VertexType>
inline uint32_t OmegaModel<VertexType>::GetSubmeshCount()
{
	if (sizeof(VertexType) == sizeof(OmegaBasicVertex))
	{
		return m_basic.m_submesh.size();
	}
	else if (sizeof(VertexType) == sizeof(OmegaVertex))
	{
		return m_typical.m_submesh.size();
	}
	else if (sizeof(VertexType) == sizeof(OmegaAnimatedVertex))
	{
		return m_animated.m_submesh.size();
	}
	else
	{
		assert(0 && "Invalid VertexType struct valid ones are OmegaBasicVertex, OmegaVertex, OmegaAnimatedVertex");
	}
}

template <class VertexType>
inline VertexType *OmegaModel<VertexType>::GetVertices(uint32_t submesh_index)
{
	if (sizeof(VertexType) == sizeof(OmegaBasicVertex))
	{
		assert(submesh_index < m_basic.m_submesh.size());
		return m_basic.m_submesh[submesh_index].m_vertices.data();
	}
	else if (sizeof(VertexType) == sizeof(OmegaVertex))
	{
		assert(submesh_index < m_typical.m_submesh.size());
		return m_typical.m_submesh[submesh_index].m_vertices.data();
	}
	else if (sizeof(VertexType) == sizeof(OmegaAnimatedVertex))
	{
		assert(submesh_index < m_animated.m_submesh.size());
		return m_animated.m_submesh[submesh_index].m_vertices.data();
	}
	else
	{
		assert(0 && "Invalid VertexType struct valid ones are OmegaBasicVertex, OmegaVertex, OmegaAnimatedVertex");
	}
	return nullptr;
}

template <class VertexType>
inline uint32_t OmegaModel<VertexType>::GetVerticesCount(uint32_t submesh_index)
{
	if (sizeof(VertexType) == sizeof(OmegaBasicVertex))
	{
		assert(submesh_index < m_basic.m_submesh.size());
		return m_basic.m_submesh[submesh_index].m_vertices.size();
	}
	else if (sizeof(VertexType) == sizeof(OmegaVertex))
	{
		assert(submesh_index < m_typical.m_submesh.size());
		return m_typical.m_submesh[submesh_index].m_vertices.size();
	}
	else if (sizeof(VertexType) == sizeof(OmegaAnimatedVertex))
	{
		assert(submesh_index < m_animated.m_submesh.size());
		return m_animated.m_submesh[submesh_index].m_vertices.size();
	}
	else
	{
		assert(0 && "Invalid VertexType struct valid ones are OmegaBasicVertex, OmegaVertex, OmegaAnimatedVertex");
	}
	return 0;
}

template <class VertexType>
inline uint32_t *OmegaModel<VertexType>::GetIndices(uint32_t submesh_index)
{

	if (sizeof(VertexType) == sizeof(OmegaBasicVertex))
	{
		assert(submesh_index < m_basic.m_submesh.size());
		return m_basic.m_submesh[submesh_index].m_indices.data();
	}
	else if (sizeof(VertexType) == sizeof(OmegaVertex))
	{
		assert(submesh_index < m_typical.m_submesh.size());
		return m_typical.m_submesh[submesh_index].m_indices.data();
	}
	else if (sizeof(VertexType) == sizeof(OmegaAnimatedVertex))
	{
		assert(submesh_index < m_animated.m_submesh.size());
		return m_animated.m_submesh[submesh_index].m_indices.data();
	}
	else
	{
		assert(0 && "Invalid VertexType struct valid ones are OmegaBasicVertex, OmegaVertex, OmegaAnimatedVertex");
	}
	return nullptr;
}

template <class VertexType>
inline uint32_t OmegaModel<VertexType>::GetIndicesCount(uint32_t submesh_index)
{
	if (sizeof(VertexType) == sizeof(OmegaBasicVertex))
	{
		assert(submesh_index < m_basic.m_submesh.size());
		return m_basic.m_submesh[submesh_index].m_indices.size();
	}
	else if (sizeof(VertexType) == sizeof(OmegaVertex))
	{
		assert(submesh_index < m_typical.m_submesh.size());
		return m_typical.m_submesh[submesh_index].m_indices.size();
	}
	else if (sizeof(VertexType) == sizeof(OmegaAnimatedVertex))
	{
		assert(submesh_index < m_animated.m_submesh.size());
		return m_animated.m_submesh[submesh_index].m_indices.size();
	}
	else
	{
		assert(0 && "Invalid VertexType struct valid ones are OmegaBasicVertex, OmegaVertex, OmegaAnimatedVertex");
	}
	return 0;
}
