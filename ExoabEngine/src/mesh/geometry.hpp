#pragma once
#include "../ShaderTypes.hpp"
#include "Float16.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory/Buffer2.hpp>

namespace Mesh {

	using namespace glm;

	// Scalar types do not STD140 alignment
	struct GeometryVertex
	{
		float x, y, z;
		float nx, ny, nz;
		float tu, tv;
	};
	struct Geometry
	{
		uint firstVertex;
		uint firstIndex;
		uint verticesCount;
		uint indicesCount;
	};
	
	// geometry_path_list - path to every mesh path (e.g. .obj, .fbx, etc)
	// out_geometry_vertices_indices_positions - an output std::vector used to provide information about where vertices and indices start
	// Note: indices will be loaded or generated for all models!
	// Indices are not SSBOs and they are 16bit
	bool LoadVerticesIndicesSSBOs(void* context, std::vector<std::string> geometry_path_list, std::vector<Geometry>& out_geometry_vertices_indices_positions, IBuffer2* pVerticesSSBO, IBuffer2* pIndicesBuffer);


};