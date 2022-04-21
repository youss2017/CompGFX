#include "geometry.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <meshoptimizer.h>
#include <StringUtils.hpp>
#include "Globals.hpp"

namespace Mesh
{
	// https://stackoverflow.com/questions/41529743/computing-the-bounding-sphere-for-a-3d-mesh-in-python
	void CalculateBoundingSphere(Geometry& geometry) {
		float lx = 0.0, hx = 0.0;
		float ly = 0.0, hy = 0.0;
		float lz = 0.0, hz = 0.0;
		for (const auto& s : geometry.mSubmeshList) {
			for (const auto& v : s.m_vertices)
			{
				lx = glm::min(lx, v.position.x); hx = glm::max(lx, v.position.x);
				ly = glm::min(ly, v.position.y); hx = glm::max(ly, v.position.y);
				lz = glm::min(lz, v.position.z); hx = glm::max(lz, v.position.z);
			}
		}
		glm::vec3 minp = glm::vec3(lx, ly, lz);
		glm::vec3 maxp = glm::vec3(hx, hy, hz);
		glm::vec3 center = (minp + maxp) / 2.0f;
		
		/*
			x x
			x x
			. .
			. .
		*/
		glm::vec3 fleft_top = glm::vec3(minp.x, maxp.y, minp.z),	fright_top = glm::vec3(maxp.x, maxp.y, minp.z);
		glm::vec3 fleft_bottom = glm::vec3(minp.x, minp.y, minp.z), fright_bottom = glm::vec3(maxp.x, minp.y, minp.z);
		glm::vec3 bleft_top = glm::vec3(minp.x, maxp.y, maxp.z),	bright_top = glm::vec3(maxp.x, maxp.y, maxp.z);
		glm::vec3 bleft_bottom = glm::vec3(minp.x, minp.y, maxp.z), bright_bottom = glm::vec3(maxp.x, minp.y, maxp.z);

		float d0 = glm::distance(center, fleft_top);
		float d1 = glm::distance(center, fleft_bottom);
		float d2 = glm::distance(center, bleft_top);
		float d3 = glm::distance(center, bleft_bottom);
		float d4 = glm::distance(center, fright_top);
		float d5 = glm::distance(center, fright_bottom);
		float d6 = glm::distance(center, bright_top);
		float d7 = glm::distance(center, bright_bottom);

		float radius = glm::max(glm::max(glm::max(glm::max(glm::max(glm::max(glm::max(d0, d1), d2), d3), d4), d5), d6), d7);
		geometry.m_bounding_sphere_center = center;
		geometry.m_bounding_sphere_radius = radius;
		geometry.box_min = minp;
		geometry.box_max = maxp;
	}

	bool LoadVerticesIndicesSSBOs(void* context, GeometryConfiguration config, std::vector<Geometry>& out_geometry_vertices_indices_positions, IBuffer2* pVerticesSSBO, IBuffer2* pIndicesBuffer)
	{
		if (!pVerticesSSBO || !pIndicesBuffer)
			return false;
		Assimp::Importer imp;
		std::vector<GeometryVertex> vertices;
		std::vector<uint32> indices;
		int meshIndex = 0;
		for (auto& _path : config.mList)
		{
			std::string &path = _path.second;
			const aiScene* scene = imp.ReadFile(path, aiProcessPreset_TargetRealtime_MaxQuality);
			if (!scene) {
				std::string errmsg = "Error parsing '" + std::string(path) + "', because: " + std::string(imp.GetErrorString());
				logerror(errmsg.c_str());
				return false;
			}
			Geometry geometry;
			
			std::stringstream infostream;
			infostream << meshIndex++ << " --> Loading Geomtry from model with " << scene->mNumMeshes << " submeshes: " << path;
			loginfos(infostream.str());
			geometry.m_bounding_sphere_center = glm::vec3(0.0);
			geometry.m_bounding_sphere_radius = 0;
			for (uint i = 0; i < scene->mNumMeshes; i++)
			{
				GeometrySubmesh submesh;
				submesh.firstVertex = vertices.size();
				submesh.firstIndex = indices.size();
				submesh.verticesCount = submesh.indicesCount = 0;
				for (uint j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
				{
					GeometryVertex v;
					v.position.x = scene->mMeshes[i]->mVertices[j].x;
					v.position.y = scene->mMeshes[i]->mVertices[j].y;
					v.position.z = scene->mMeshes[i]->mVertices[j].z;
					v.normal.x = scene->mMeshes[i]->mNormals[j].x;
					v.normal.y = scene->mMeshes[i]->mNormals[j].y;
					v.normal.z = scene->mMeshes[i]->mNormals[j].z;
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

				submesh.verticesCount += scene->mMeshes[i]->mNumVertices;
				submesh.indicesCount += scene->mMeshes[i]->mNumFaces * 3;
				submesh.m_vertices = vertices;
				submesh.m_indices = indices;
				geometry.mSubmeshList.push_back(submesh);
			}
			imp.FreeScene();
			CalculateBoundingSphere(geometry);
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
		IBuffer2 vertices_ssbo = Buffer2_Create(Global::Context, BUFFER_TYPE_STORAGE, vertices.size() * sizeof(GeometryVertex), BufferMemoryType::GPU_ONLY, false, false);
		IBuffer2 indices_buffer = Buffer2_Create(Global::Context, BUFFER_TYPE_INDEX, indices.size() * sizeof(uint32), BufferMemoryType::GPU_ONLY, false, false);
		Buffer2_UploadData(vertices_ssbo, (char8_t*)vertices.data(), 0, vertices.size() * sizeof(GeometryVertex));
		Buffer2_UploadData(indices_buffer, (char8_t*)indices.data(), 0, indices.size() * sizeof(uint32));
		*pVerticesSSBO = vertices_ssbo;
		*pIndicesBuffer = indices_buffer;
		return true;
	}

	void LoadVerticesIndicesBONE(void* context, std::vector<std::string> paths, std::vector<SkinnedMesh>& OutSkinnedMesh, IBuffer2* pOutVertices, IBuffer2* pOutIndices)
	{
		size_t verticesSize = 0;
		size_t indicesSize = 0;
		for (auto& path : paths) {
			SkinnedMesh mesh(path);
			verticesSize += mesh.mVertices.size() * sizeof(Mesh::VertexBone);
			indicesSize += mesh.mIndices.size() * sizeof(uint32_t);
			OutSkinnedMesh.push_back(mesh);
		}
		*pOutVertices = Buffer2_Create(Global::Context, BUFFER_TYPE_STORAGE, verticesSize, BufferMemoryType::GPU_ONLY, false, false);
		*pOutIndices = Buffer2_Create(Global::Context, BUFFER_TYPE_INDEX, indicesSize, BufferMemoryType::GPU_ONLY, false, false);
		size_t verticesOffset = 0;
		size_t indicesOffset = 0;
		for (auto& skinnedMesh : OutSkinnedMesh) {
			verticesSize = skinnedMesh.mVertices.size() * sizeof(Mesh::VertexBone);
			indicesSize = skinnedMesh.mIndices.size() * sizeof(uint32_t);
			Buffer2_UploadData(*pOutVertices, (char8_t*)skinnedMesh.mVertices.data(), verticesOffset, verticesSize);
			Buffer2_UploadData(*pOutIndices, (char8_t*)skinnedMesh.mIndices.data(), indicesOffset, indicesSize);
		}
	
	}

	void GeometryConfiguration::Load(const std::string& configPath)
	{
		std::string config = Utils::LoadTextFile(configPath);
		std::vector<std::string> config_split = Utils::StringSplitter("\n", config);
		int line = -1;
		for (auto& geo : config_split) {
			line++;
			geo = Utils::StrTrim(geo);
			if (Utils::StrStartsWith(geo, "//#")) {
				std::string geosub = geo.data() + 3;
				geosub = Utils::StrTrim(geosub);
				std::vector<std::string> geo_split = Utils::StringSplitter(" ", geosub);
				if (geo_split.size() != 2) {
					char log[150];
					sprintf(log, "Syntax error in Geometry Configuration file [%s] at line %i", configPath.c_str(), line + 1);
					log_error(log, __FILE__, __LINE__);
					throw std::runtime_error(log);
				}
				mList.insert(std::make_pair(std::stoi(geo_split[1]), geo_split[0]));
			}
		}
	}

}