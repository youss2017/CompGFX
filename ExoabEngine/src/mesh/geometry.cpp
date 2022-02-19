#include "geometry.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>>

namespace Mesh
{

	bool LoadVerticesIndicesSSBOs(void* context, std::vector<std::string> geometry_path_list, std::vector<Geometry>& out_geometry_vertices_indices_positions, IBuffer2* pVerticesSSBO, IBuffer2* pIndicesBuffer)
	{
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
			}
			Geometry geomtry;
			geomtry.firstVertex = vertices.size();
			geomtry.firstIndex = indices.size();
			geomtry.verticesCount = geomtry.indicesCount = 0;
			std::stringstream infostream;
			infostream << meshIndex++ << " --> Loading Geomtry from model with " << scene->mNumMeshes << " submeshes: " << path << "\n";
			lograws(infostream.str());
			for (int i = 0; i < scene->mNumMeshes; i++)
			{
				vertices.reserve(scene->mMeshes[i]->mNumVertices);
				indices.reserve(scene->mMeshes[i]->mNumFaces * 3);
				for (int j = 0; j < scene->mMeshes[i]->mNumVertices; j++)
				{
					GeometryVertex v;
					v.x = scene->mMeshes[i]->mVertices[j].x;
					v.y = scene->mMeshes[i]->mVertices[j].y;
					v.z = scene->mMeshes[i]->mVertices[j].z;
					v.nx = int8(scene->mMeshes[i]->mNormals[j].x * 127.0);
					v.ny = int8(scene->mMeshes[i]->mNormals[j].y * 127.0);
					v.nz = int8(scene->mMeshes[i]->mNormals[j].z * 127.0);
					if (scene->mMeshes[i]->mTextureCoords[0]) {
						v.tu = Float16(scene->mMeshes[i]->mTextureCoords[0][j].x);
						v.tv = Float16(scene->mMeshes[i]->mTextureCoords[0][j].y);
					}
					else
						v.tu = v.tv = 0;
					vertices.push_back(v);
				}
				for (int j = 0; j < scene->mMeshes[i]->mNumFaces; j++)
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

	}

}