#include "geometry.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <meshoptimizer/src/meshoptimizer.h>

namespace Mesh
{

	bool LoadVerticesIndicesSSBOs(void* context, std::vector<std::string> geometry_path_list, std::vector<Geometry>& out_geometry_vertices_indices_positions, IBuffer2* pVerticesSSBO, IBuffer2* pIndicesBuffer)
	{
		if (!pVerticesSSBO || !pIndicesBuffer)
			return false;
		Assimp::Importer imp;
		std::vector<GeometryVertex> vertices;
		std::vector<uint16> indices;
		int meshIndex = 0;
		for (auto& path : geometry_path_list)
		{
			const aiScene* scene = imp.ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality);
			if (!scene) {
				std::string errmsg = "Error parsing '" + std::string(path) + "', because: " + std::string(imp.GetErrorString());
				logerror(errmsg.c_str());
				return false;
			}
			Geometry geometry;
			geometry.firstVertex = vertices.size();
			geometry.firstIndex = indices.size();
			geometry.verticesCount = geometry.indicesCount = 0;
			std::stringstream infostream;
			infostream << meshIndex++ << " --> Loading Geomtry from model with " << scene->mNumMeshes << " submeshes: " << path;
			loginfos(infostream.str());
			for (uint i = 0; i < scene->mNumMeshes; i++)
			{
				vertices.reserve(scene->mMeshes[i]->mNumVertices);
				indices.reserve(scene->mMeshes[i]->mNumFaces * 3);
				for (uint j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
				{
					GeometryVertex v;
#if 1
					v.x = meshopt_quantizeHalf(scene->mMeshes[i]->mVertices[j].x);
					v.y = meshopt_quantizeHalf(scene->mMeshes[i]->mVertices[j].y);
					v.z = meshopt_quantizeHalf(scene->mMeshes[i]->mVertices[j].z);
					v.nx = uint8_t(scene->mMeshes[i]->mNormals[j].x * 127.0 + 127.0);
					v.ny = uint8_t(scene->mMeshes[i]->mNormals[j].y * 127.0 + 127.0);
					v.nz = uint8_t(scene->mMeshes[i]->mNormals[j].z * 127.0 + 127.0);
#else
					v.x = (scene->mMeshes[i]->mVertices[j].x);
					v.y = (scene->mMeshes[i]->mVertices[j].y);
					v.z = (scene->mMeshes[i]->mVertices[j].z);
					v.nx = (scene->mMeshes[i]->mNormals[j].x);
					v.ny = (scene->mMeshes[i]->mNormals[j].y);
					v.nz = (scene->mMeshes[i]->mNormals[j].z);
#endif
					if (scene->mMeshes[i]->mTextureCoords[0]) {
#if 1
						v.tu = meshopt_quantizeHalf(scene->mMeshes[i]->mTextureCoords[0][j].x);
						v.tv = meshopt_quantizeHalf(scene->mMeshes[i]->mTextureCoords[0][j].y);
#else
						v.tu = (scene->mMeshes[i]->mTextureCoords[0][j].x);
						v.tv = (scene->mMeshes[i]->mTextureCoords[0][j].y);
#endif
					}
					else
						v.tu = v.tv = 0;
					vertices.push_back(v);
				}
				for (uint j = 0; j < scene->mMeshes[i]->mNumFaces; j++)
				{
					indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[0]);
					indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[1]);
					indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[2]);
				}
				geometry.verticesCount += scene->mMeshes[i]->mNumVertices;
				geometry.indicesCount += scene->mMeshes[i]->mNumFaces * 3;
				geometry.m_vertices = vertices;
				geometry.m_indices = indices;
			}
			out_geometry_vertices_indices_positions.push_back(geometry);
			imp.FreeScene();
			geometry.m_bounding_sphere_center = glm::vec3(0.0);
			for(auto& vertex : geometry.m_vertices)
				geometry.m_bounding_sphere_center += glm::vec3(meshopt_quantizeFloat(vertex.x, 2), meshopt_quantizeFloat(vertex.y, 2.0), meshopt_quantizeFloat(vertex.z, 2.0));
			geometry.m_bounding_sphere_radius = 0;
			for (auto& vertex : geometry.m_vertices) {
				glm::vec3 pos = glm::vec3(meshopt_quantizeFloat(vertex.x, 2), meshopt_quantizeFloat(vertex.y, 2.0), meshopt_quantizeFloat(vertex.z, 2.0));
				if(abs(glm::distance(pos, geometry.m_bounding_sphere_center) > geometry.m_bounding_sphere_radius))
					geometry.m_bounding_sphere_radius = abs(glm::distance(pos, geometry.m_bounding_sphere_center));
			}
		}
		vertices.shrink_to_fit();
		indices.shrink_to_fit();
		// Optimize mesh
		//size_t index_count = indices.size();
		//std::vector<unsigned int> remap(index_count); // allocate temporary memory for the remap table
		//size_t vertex_count = meshopt_generateVertexRemap(&remap[0], nullptr, index_count, vertices.data(), index_count, sizeof(GeometryVertex));
		//
		//meshopt_remapIndexBuffer(indices.data(), (uint16_t*)nullptr, index_count, &remap[0]);
		//meshopt_remapVertexBuffer(vertices.data(), &unindexed_vertices[0], index_count, sizeof(Vertex), &remap[0]);

		std::stringstream memory_info;
		memory_info << "Using about " << (vertices.size() * sizeof(GeometryVertex) / 1024.0) << " kb for vertices and " << (indices.size() * sizeof(uint16) / 1024.0) << " kb for indices.";
		loginfos(memory_info.str());
		IBuffer2 vertices_ssbo = Buffer2_Create(context, BufferType::StorageBuffer, vertices.size() * sizeof(GeometryVertex), BufferMemoryType::GPU_ONLY);
		IBuffer2 indices_buffer = Buffer2_Create(context, BufferType::IndexBuffer, indices.size() * sizeof(uint16), BufferMemoryType::GPU_ONLY);
		Buffer2_UploadData(vertices_ssbo, (char8_t*)vertices.data(), 0, vertices.size() * sizeof(GeometryVertex));
		Buffer2_UploadData(indices_buffer, (char8_t*)indices.data(), 0, indices.size() * sizeof(uint16));
		*pVerticesSSBO = vertices_ssbo;
		*pIndicesBuffer = indices_buffer;
		return true;
	}

}