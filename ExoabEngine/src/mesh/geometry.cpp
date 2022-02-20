#include "geometry.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

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
			Geometry geomtry;
			geomtry.firstVertex = vertices.size();
			geomtry.firstIndex = indices.size();
			geomtry.verticesCount = geomtry.indicesCount = 0;
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
					v.x = scene->mMeshes[i]->mVertices[j].x;
					v.y = scene->mMeshes[i]->mVertices[j].y;
					v.z = scene->mMeshes[i]->mVertices[j].z;
					v.nx = scene->mMeshes[i]->mNormals[j].x ;
					v.ny = scene->mMeshes[i]->mNormals[j].y ;
					v.nz = scene->mMeshes[i]->mNormals[j].z ;
					if (scene->mMeshes[i]->mTextureCoords[0]) {
						v.tu = scene->mMeshes[i]->mTextureCoords[0][j].x;
						v.tv = scene->mMeshes[i]->mTextureCoords[0][j].y;
					}
					else
						;// v.tu_tv = 0;
					vertices.push_back(v);
				}
				for (uint j = 0; j < scene->mMeshes[i]->mNumFaces; j++)
				{
					indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[0]);
					indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[1]);
					indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[2]);
				}
				geomtry.verticesCount += scene->mMeshes[i]->mNumVertices;
				geomtry.indicesCount += scene->mMeshes[i]->mNumFaces * 3;
			}
			out_geometry_vertices_indices_positions.push_back(geomtry);
			imp.FreeScene();
		}
		vertices.shrink_to_fit();
		indices.shrink_to_fit();
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