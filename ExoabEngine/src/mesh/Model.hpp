#pragma once
#include "mesh.hpp"
#include "../graphics/RenderState.hpp"
/*
	VertexType can be any of the following:
		OmegaBasicVertex
		OmegaVertex
		OmegaAnimatedVertex
*/

template <class VertexType>
class OmegaModel {
public:
	OmegaModel(GraphicsContext context, const char* mesh_path);
	~OmegaModel();
public:
	inline uint32_t    GetSubmeshCount();
	inline VertexType* GetVertices(uint32_t submesh_index = 0);
	inline uint32_t    GetVerticesCount(uint32_t submesh_index = 0);
	inline uint32_t*   GetIndices(uint32_t submesh_index = 0);
	inline uint32_t    GetIndicesCount(uint32_t submesh_index = 0);
	inline RenderState& GetRenderState(uint32_t submesh_index = 0) { return m_state[submesh_index]; }
private:
	OmegaBasicMesh m_basic;
	OmegaMesh m_typical;
	OmegaAnimatedMesh m_animated;
	
	std::vector<IGPUBuffer> m_buffers;
	std::vector<RenderState> m_state;
};

#include "Model.ipp"
