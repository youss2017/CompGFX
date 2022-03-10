#include "geometry.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <meshoptimizer/src/meshoptimizer.h>

namespace Mesh
{

	static void computeBoundingSphere(float result[4], const glm::vec3* points, size_t count)
	{
		assert(count > 0);

		// find extremum points along all 3 axes; for each axis we get a pair of points with min/max coordinates
		size_t pmin[3] = { 0, 0, 0 };
		size_t pmax[3] = { 0, 0, 0 };

		for (size_t i = 0; i < count; ++i)
		{
			const glm::vec3 p = points[i];

			for (int axis = 0; axis < 3; ++axis)
			{
				pmin[axis] = (p[axis] < points[pmin[axis]][axis]) ? i : pmin[axis];
				pmax[axis] = (p[axis] > points[pmax[axis]][axis]) ? i : pmax[axis];
			}
		}

		// find the pair of points with largest distance
		float paxisd2 = 0;
		int paxis = 0;

		for (int axis = 0; axis < 3; ++axis)
		{
			const glm::vec3 p1 = points[pmin[axis]];
			const glm::vec3 p2 = points[pmax[axis]];

			float d2 = (p2[0] - p1[0]) * (p2[0] - p1[0]) + (p2[1] - p1[1]) * (p2[1] - p1[1]) + (p2[2] - p1[2]) * (p2[2] - p1[2]);

			if (d2 > paxisd2)
			{
				paxisd2 = d2;
				paxis = axis;
			}
		}

		// use the longest segment as the initial sphere diameter
		const glm::vec3 p1 = points[pmin[paxis]];
		const glm::vec3 p2 = points[pmax[paxis]];

		float center[3] = { (p1[0] + p2[0]) / 2, (p1[1] + p2[1]) / 2, (p1[2] + p2[2]) / 2 };
		float radius = sqrtf(paxisd2) / 2;

		// iteratively adjust the sphere up until all points fit
		for (size_t i = 0; i < count; ++i)
		{
			const glm::vec3 p = points[i];
			float d2 = (p[0] - center[0]) * (p[0] - center[0]) + (p[1] - center[1]) * (p[1] - center[1]) + (p[2] - center[2]) * (p[2] - center[2]);

			if (d2 > radius * radius)
			{
				float d = sqrtf(d2);
				assert(d > 0);

				float k = 0.5f + (radius / d) / 2;

				center[0] = center[0] * k + p[0] * (1 - k);
				center[1] = center[1] * k + p[1] * (1 - k);
				center[2] = center[2] * k + p[2] * (1 - k);
				radius = (radius + d) / 2;
			}
		}

		result[0] = center[0];
		result[1] = center[1];
		result[2] = center[2];
		result[3] = radius;
	}


	bool LoadVerticesIndicesSSBOs(void* context, std::vector<std::string> geometry_path_list, std::vector<Geometry>& out_geometry_vertices_indices_positions, IBuffer2* pVerticesSSBO, IBuffer2* pIndicesBuffer)
	{
		if (!pVerticesSSBO || !pIndicesBuffer)
			return false;
		Assimp::Importer imp;
		std::vector<GeometryVertex> vertices;
		std::vector<uint32> indices;
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
			geometry.m_bounding_sphere_center = glm::vec3(0.0);
			geometry.m_bounding_sphere_radius = 0;
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
					v.position.x = scene->mMeshes[i]->mVertices[j].x;
					v.position.y = scene->mMeshes[i]->mVertices[j].y;
					v.position.z = scene->mMeshes[i]->mVertices[j].z;
					v.position.w = 1.0f;
					v.normal.x = scene->mMeshes[i]->mNormals[j].x;
					v.normal.y = scene->mMeshes[i]->mNormals[j].y;
					v.normal.z = scene->mMeshes[i]->mNormals[j].z;
					glm::vec3 position = glm::vec3(scene->mMeshes[i]->mVertices[j].x, scene->mMeshes[i]->mVertices[j].y, scene->mMeshes[i]->mVertices[j].z);
					geometry.m_bounding_sphere_center += position;
					geometry.m_bounding_sphere_center /= 2.0;
					if (glm::distance(position, geometry.m_bounding_sphere_center) > geometry.m_bounding_sphere_radius)
						geometry.m_bounding_sphere_radius = glm::distance(position, geometry.m_bounding_sphere_center);
					if (scene->mMeshes[i]->mTextureCoords[0]) {
						v.tu = scene->mMeshes[i]->mTextureCoords[0][j].x;
						v.tv = scene->mMeshes[i]->mTextureCoords[0][j].y;
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
			imp.FreeScene();
			out_geometry_vertices_indices_positions.push_back(geometry);
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
		memory_info << "Using about " << (vertices.size() * sizeof(GeometryVertex) / 1024.0) << " kb for vertices and " << (indices.size() * sizeof(uint32) / 1024.0) << " kb for indices.";
		loginfos(memory_info.str());
		IBuffer2 vertices_ssbo = Buffer2_Create(context, BufferType::StorageBuffer, vertices.size() * sizeof(GeometryVertex), BufferMemoryType::GPU_ONLY);
		IBuffer2 indices_buffer = Buffer2_Create(context, BufferType::IndexBuffer, indices.size() * sizeof(uint32), BufferMemoryType::GPU_ONLY);
		Buffer2_UploadData(vertices_ssbo, (char8_t*)vertices.data(), 0, vertices.size() * sizeof(GeometryVertex));
		Buffer2_UploadData(indices_buffer, (char8_t*)indices.data(), 0, indices.size() * sizeof(uint32));
		*pVerticesSSBO = vertices_ssbo;
		*pIndicesBuffer = indices_buffer;
		return true;
	}

	void LoadVerticesIndicesBONE(std::string path)
	{
		Assimp::Importer imp;
		const aiScene* scene = imp.ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality);
		if (!scene) {
			std::string errmsg = "Error parsing '" + std::string(path) + "', because: " + std::string(imp.GetErrorString());
			logerror(errmsg.c_str());
		}
		std::vector<GeometryVertexBone> vertices;
		std::vector<uint32> indices;

		for (uint i = 0; i < scene->mNumMeshes; i++)
		{
			vertices.reserve(scene->mMeshes[i]->mNumVertices);
			indices.reserve(scene->mMeshes[i]->mNumFaces * 3);
			for (uint j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
			{
				GeometryVertexBone v;
				v.position.x = scene->mMeshes[i]->mVertices[j].x;
				v.position.y = scene->mMeshes[i]->mVertices[j].y;
				v.position.z = scene->mMeshes[i]->mVertices[j].z;
				v.position.w = 1.0f;
				v.normal.x = scene->mMeshes[i]->mNormals[j].x;
				v.normal.y = scene->mMeshes[i]->mNormals[j].y;
				v.normal.z = scene->mMeshes[i]->mNormals[j].z;
				v.boneIDs = glm::ivec4(-1);
				glm::vec3 position = glm::vec3(scene->mMeshes[i]->mVertices[j].x, scene->mMeshes[i]->mVertices[j].y, scene->mMeshes[i]->mVertices[j].z);
				if (scene->mMeshes[i]->mTextureCoords[0]) {
					v.tu = scene->mMeshes[i]->mTextureCoords[0][j].x;
					v.tv = scene->mMeshes[i]->mTextureCoords[0][j].y;
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

			for (uint j = 0; j < scene->mMeshes[i]->mNumBones; j++) {
				aiBone* bone = scene->mMeshes[i]->mBones[j];
				std::cout << bone->mName.C_Str() << " ";
				for (uint w = 0; w < bone->mNumWeights; w++) {
					aiVertexWeight weight = bone->mWeights[w];
					GeometryVertexBone& vertex = vertices[weight.mVertexId];
					// make sure there are no duplicates.
					if (vertex.boneIDs[0] == j || vertex.boneIDs[1] == j || vertex.boneIDs[2] == j || vertex.boneIDs[3] == j)
						continue;
					if (vertex.boneIDs[0] == -1) {
						vertex.boneIDs[0] = j;
						vertex.boneWeights[0] = weight.mWeight;
					}
					else if (vertex.boneIDs[1] == -1) {
						vertex.boneIDs[1] = j;
						vertex.boneWeights[1] = weight.mWeight;
					}
					else if (vertex.boneIDs[2] == -1) {
						vertex.boneIDs[2] = j;
						vertex.boneWeights[2] = weight.mWeight;
					}
					else if (vertex.boneIDs[3] == -1) {
						vertex.boneIDs[3] = j;
						vertex.boneWeights[3] = weight.mWeight;
					}
					else {
						log_error("Max supported joints/bones is 4!", __FILE__, __LINE__);
					}
				}
			}

			for (auto& vertex : vertices) {
				if (vertex.boneIDs[0] == -1) {
					vertex.boneIDs[0] = 0;
					vertex.boneWeights[0] = 0.0f;
				}
				if (vertex.boneIDs[1] == -1) {
					vertex.boneIDs[1] = 0;
					vertex.boneWeights[1] = 0.0f;
				}
				if (vertex.boneIDs[2] == -1) {
					vertex.boneIDs[2] = 0;
					vertex.boneWeights[2] = 0.0f;
				}
				if (vertex.boneIDs[3] == -1) {
					vertex.boneIDs[3] = 0;
					vertex.boneWeights[3] = 0.0f;
				}
			}
		}
	}

}