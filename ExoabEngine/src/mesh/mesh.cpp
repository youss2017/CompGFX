#include "mesh.hpp"
#include "../utils/Logger.hpp"
#include <assimp/DefaultLogger.hpp>

OmegaBasicMesh Omega_LoadBasicMesh(const char* mesh_path)
{
    //Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, aiDefaultLogStream_STDOUT);
    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(mesh_path, aiProcessPreset_TargetRealtime_MaxQuality);
    if(!scene) {
        std::string errmsg = "Error parsing '" + std::string(mesh_path) + "', because: " + std::string(imp.GetErrorString());
        logerror(errmsg.c_str());
        return OmegaBasicMesh();
    }
    OmegaBasicMesh mesh;
    mesh.m_name = "NULL";//scene->mName.C_Str();
    mesh.m_submesh.resize(scene->mNumMeshes);
    for(uint32_t i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* ai_submesh = scene->mMeshes[i];
        OmegaBasicSubMesh submesh;
        submesh.m_submesh_name = ai_submesh->mName.C_Str();
        submesh.m_vertices.resize(ai_submesh->mNumVertices);
        for(uint32_t j = 0; j < ai_submesh->mNumVertices; j++) {
            OmegaBasicVertex vertex;
            vertex.m_position = *((glm::fvec3*)&ai_submesh->mVertices[j]);
            vertex.m_normal = *((glm::fvec3*)&ai_submesh->mNormals[j]);
            if(ai_submesh->mTextureCoords[0])
                vertex.m_uv   = *((glm::fvec2*)&ai_submesh->mTextureCoords[0][j]);
            else
                vertex.m_uv   = glm::fvec2(0.0f);
            submesh.m_vertices[j] = vertex;
        }
        submesh.m_indices.resize(ai_submesh->mNumFaces * 3);
        for (uint32_t j = 0, w = 0; j < ai_submesh->mNumFaces; j++) {
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[0];
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[1];
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[2];
        }
        mesh.m_submesh[i] = submesh;
    }
    imp.FreeScene();
    mesh.m_name = mesh_path;
    return mesh;
}

OmegaMesh Omega_LoadMesh(const char* mesh_path)
{
    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(mesh_path, aiProcessPreset_TargetRealtime_MaxQuality);
    if(!scene) {
        std::string errmsg = "Error parsing '" + std::string(mesh_path) + "', because: " + std::string(imp.GetErrorString());
        logerror(errmsg.c_str());
        return OmegaMesh();
    }
    OmegaMesh mesh;
    mesh.m_name = "NULL";// scene->mName.C_Str();
    mesh.m_submesh.resize(scene->mNumMeshes);
    for(uint32_t i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* ai_submesh = scene->mMeshes[i];
        OmegaSubMesh submesh;
        submesh.m_submesh_name = ai_submesh->mName.C_Str();
        submesh.m_vertices.resize(ai_submesh->mNumVertices);
        for(uint32_t j = 0; j < ai_submesh->mNumVertices; j++) {
            OmegaVertex vertex;
            vertex.m_position = *((glm::fvec3*)&ai_submesh->mVertices[j]);
            vertex.m_normal   = *((glm::fvec3*)&ai_submesh->mNormals[j]);
            vertex.m_tanget   = *((glm::fvec3*)&ai_submesh->mTangents[j]);
            vertex.m_bitanget = *((glm::fvec3*)&ai_submesh->mBitangents[j]);
            if(ai_submesh->mTextureCoords[0])
                vertex.m_uv   = *((glm::fvec2*)&ai_submesh->mTextureCoords[0][j]);
            else
                vertex.m_uv   = glm::fvec2(0.0f);
            submesh.m_vertices[j] = vertex;
        }
        submesh.m_indices.resize(ai_submesh->mNumFaces * 3);
        for (uint32_t j = 0, w = 0; j < ai_submesh->mNumFaces; j++) {
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[0];
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[1];
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[2];
        }
        mesh.m_submesh.push_back(submesh);
    }
    imp.FreeScene();
    return mesh;
}

/////////////////////////////// ANIMATED MESH

OmegaAnimatedMesh Omega_LoadAnimatedMesh(const char* mesh_path)
{
    Assimp::Importer imp;
    const aiScene* scene = imp.ReadFile(mesh_path, aiProcessPreset_TargetRealtime_MaxQuality);
    if(!scene) {
        std::string errmsg = "Error parsing '" + std::string(mesh_path) + "', because: " + std::string(imp.GetErrorString());
        logerror(errmsg.c_str());
        return OmegaAnimatedMesh();
    }
    OmegaAnimatedMesh mesh;
    mesh.m_name = "NULL";// scene->mName.C_Str();
    mesh.m_submesh.resize(scene->mNumMeshes);

    std::map<std::string, OmegaBoneInfo> BoneInfoMap;
    int BoneCounter = 0;

    auto SetVertexBoneData = [](OmegaAnimatedVertex& vertex, int boneID, float weight) throw() -> void
    {
        for (uint32_t i = 0; i < 4; ++i)
        {
            if (vertex.m_boneIDs[i] < 0)
            {
                vertex.m_boneWeights[i] = weight;
                vertex.m_boneIDs[i] = boneID;
                break;
            }
        }
    };

    auto ExtractBoneInformation = [&BoneInfoMap, &BoneCounter, SetVertexBoneData]
    (std::vector<OmegaAnimatedVertex>& vertices, aiMesh* submesh, const aiScene* scene) throw() -> void {
        int boneID = -1;
        for (uint32_t boneIndex = 0; boneIndex < submesh->mNumBones; boneIndex++) {
            std::string boneName = submesh->mBones[boneIndex]->mName.C_Str();
            if (BoneInfoMap.find(boneName) == BoneInfoMap.end())
            {
                OmegaBoneInfo newBoneInfo;
                newBoneInfo.m_ID = BoneCounter;
                newBoneInfo.m_offset = *(glm::mat4*)&submesh->mBones[boneIndex]->mOffsetMatrix;
                BoneInfoMap[boneName] = newBoneInfo;
                boneID = BoneCounter;
                BoneCounter++;
            }
            else
            {
                boneID = BoneInfoMap[boneName].m_ID;
            }
            assert(boneID != -1);
            auto weights = submesh->mBones[boneIndex]->mWeights;
            uint32_t numWeights = submesh->mBones[boneIndex]->mNumWeights;

            for (uint32_t weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                size_t vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    };

    for(uint32_t i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* ai_submesh = scene->mMeshes[i];
        OmegaAnimatedSubMesh submesh;
        submesh.m_submesh_name = ai_submesh->mName.C_Str();
        submesh.m_vertices.resize(ai_submesh->mNumVertices);
        // TODO: Figure out a way to deal with mesh that has some submeshes without animation data. 
        assert(ai_submesh->HasBones() && "When loading an animated mesh, it must have actual animation data in all submeshes!");
        for(uint32_t j = 0; j < ai_submesh->mNumVertices; j++) {
            OmegaAnimatedVertex vertex;
            vertex.m_position    = *((glm::fvec3*)&ai_submesh->mVertices[j]);
            vertex.m_normal      = *((glm::fvec3*)&ai_submesh->mNormals[j]);
            vertex.m_tanget      = *((glm::fvec3*)&ai_submesh->mTangents[j]);
            vertex.m_bitanget    = *((glm::fvec3*)&ai_submesh->mBitangents[j]);
            vertex.m_boneWeights = glm::fvec4(0.0f); // set them to default values
            vertex.m_boneIDs     = glm::ivec4(-1);
            if(ai_submesh->mTextureCoords[0])
                vertex.m_uv      = *((glm::fvec2*)&ai_submesh->mTextureCoords[0][j]);
            else
                vertex.m_uv      = glm::fvec2(0.0f);
            submesh.m_vertices[j] = vertex;
        }
        submesh.m_indices.resize(ai_submesh->mNumFaces * 3);
        for (uint32_t j = 0, w = 0; j < ai_submesh->mNumFaces; j++) {
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[0];
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[1];
            submesh.m_indices[w++] = ai_submesh->mFaces[j].mIndices[2];
        }
        ExtractBoneInformation(submesh.m_vertices, ai_submesh, scene);
        mesh.m_submesh.push_back(submesh);
    }
    mesh.m_BoneInfoMap = BoneInfoMap;
    mesh.m_BoneCount = BoneCounter;
    imp.FreeScene();
    return mesh;
}
