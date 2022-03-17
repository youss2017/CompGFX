#pragma once
#include "../application/ShaderTypes.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory/Buffer2.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include "SkinnedMesh.hpp"

namespace Mesh {

	using namespace glm;

	struct Joint {
		std::string mName;
		mat4 mTransformation;
		std::vector<Joint> mChildern;
	};

	struct Animation {
		double mDuration;
		double mTicksPerSecond;

	};

	struct GeometryVertex
	{
		vec3 position;
		vec3 normal;
		float tu, tv;
	};

	struct GeometrySubmesh{
		uint firstVertex;
		uint firstIndex;
		uint verticesCount;
		uint indicesCount;
		std::vector<GeometryVertex> m_vertices;
		std::vector<uint32_t> m_indices;
	};

	struct Geometry
	{
		std::vector<GeometrySubmesh> mSubmeshList;
		glm::vec3 m_bounding_sphere_center;
		float m_bounding_sphere_radius;
	};
	
	// geometry_path_list - path to every mesh path (e.g. .obj, .fbx, etc)
	// out_geometry_vertices_indices_positions - an output std::vector used to provide information about where vertices and indices start
	// Note: indices will be loaded or generated for all models!
	// Indices are not SSBOs and they are 16bit
	bool LoadVerticesIndicesSSBOs(void* context, std::vector<std::string> geometry_path_list, std::vector<Geometry>& out_geometry_vertices_indices_positions, IBuffer2* pVerticesSSBO, IBuffer2* pIndicesBuffer);

	void LoadVerticesIndicesBONE(void* context, std::vector<std::string> path, std::vector<Mesh::SkinnedMesh>& OutSkinnedMesh, IBuffer2* pOutVertices, IBuffer2* pOutIndices);


};