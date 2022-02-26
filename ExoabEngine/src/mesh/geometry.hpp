#pragma once
#include "../application/ShaderTypes.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory/Buffer2.hpp>

namespace Mesh {

	using namespace glm;

	struct GeometryVertex
	{
		uint16_t x, y, z;
		uint8_t nx, ny, nz, nw;
		uint16_t tu, tv;
	};
	struct Geometry
	{
		uint firstVertex;
		uint firstIndex;
		uint verticesCount;
		uint indicesCount;
		std::vector<GeometryVertex> m_vertices;
		std::vector<uint16_t> m_indices;
	};
	
	// geometry_path_list - path to every mesh path (e.g. .obj, .fbx, etc)
	// out_geometry_vertices_indices_positions - an output std::vector used to provide information about where vertices and indices start
	// Note: indices will be loaded or generated for all models!
	// Indices are not SSBOs and they are 16bit
	bool LoadVerticesIndicesSSBOs(void* context, std::vector<std::string> geometry_path_list, std::vector<Geometry>& out_geometry_vertices_indices_positions, IBuffer2* pVerticesSSBO, IBuffer2* pIndicesBuffer);


};