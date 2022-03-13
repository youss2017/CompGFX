#include "SkinnedMesh.hpp"
#include <assimp/postprocess.h>
#include <iostream>

namespace Mesh {

    using namespace std;

    SkinnedMesh::SkinnedMesh(const string& Filename)
    {
        bool Ret = false;
        Assimp::Importer imp;
        mScene = imp.ReadFile(Filename.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
        imp.GetOrphanedScene();
        if (!mScene) {
            char elog[100];
            elog[0] = 0;
            sprintf(elog, "Error parsing '%s': '%s'\n", Filename.c_str(), imp.GetErrorString());
            throw std::runtime_error(elog);
        }
        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

      //  // Populate the vertex attribute vectors
      //  for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
      //      VertexBone v;
      //      const aiVector3D& pPos = paiMesh->mVertices[i];
      //      //m_Positions.push_back(glm::vec3(pPos.x, pPos.y, pPos.z));
      //      v.m_Positions = glm::vec4(pPos.x, pPos.y, pPos.z, 1.0);
      //
      //      const aiVector3D& pNormal = paiMesh->mNormals[i];
      //      //m_Normals.push_back(glm::vec3(pNormal.x, pNormal.y, pNormal.z));
      //      v.m_Normals = glm::vec4(pNormal.x, pNormal.y, pNormal.z, 0.0);
      //
      //      const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : Zero3D;
      //      //m_TexCoords.push_back(glm::vec2(pTexCoord.x, pTexCoord.y));
      //      v.m_TexCoords = glm::vec4(pTexCoord.x, pTexCoord.y, 0.0f, 0.0f);
      //      v.m_BoneWeights = glm::vec4(0.0);
      //      v.m_BoneIDs = glm::ivec4(-1);
      //      mVertices.push_back(v);
      //  }
      //
      //
      //  // Populate the index buffer
      //  for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
      //      const aiFace& Face = paiMesh->mFaces[i];
      //      //        printf("num indices %d\n", Face.mNumIndices);
      //      //        assert(Face.mNumIndices == 3);
      //      mIndices.push_back(Face.mIndices[0]);
      //      mIndices.push_back(Face.mIndices[1]);
      //      mIndices.push_back(Face.mIndices[2]);
      //  }
      //
      //  // load bone info into vertices
      //  for (unsigned int i = 0; i < paiMesh->mNumBones; i++) {
      //      aiBone* bone = paiMesh->mBones[i];
      //      aiVertexWeight* weight = bone->mWeights;
      //      VertexBone& v = mVertices[weight->mVertexId];
      //      if (v.m_BoneIDs[0] == -1) {
      //          v.m_BoneIDs[0] = i;
      //          v.m_BoneWeights[0] = weight->mWeight;
      //      }
      //      else if (v.m_BoneIDs[1] == -1) {
      //          v.m_BoneIDs[1] = i;
      //          v.m_BoneWeights[1] = weight->mWeight;
      //      }
      //      else if (v.m_BoneIDs[2] == -1) {
      //          v.m_BoneIDs[2] = i;
      //          v.m_BoneWeights[2] = weight->mWeight;
      //      }
      //      else if (v.m_BoneIDs[3] == -1) {
      //          v.m_BoneIDs[3] = i;
      //          v.m_BoneWeights[3] = weight->mWeight;
      //      }
      //      else {
      //          assert(0);
      //      }
      //  }
      //
      //  for (auto& v : mVertices) {
      //      if (v.m_BoneIDs[0] == -1) {
      //          v.m_BoneIDs[0] = 0;
      //          v.m_BoneWeights[0] = 0.0f;
      //      }
      //      if (v.m_BoneIDs[1] == -1) {
      //          v.m_BoneIDs[1] = 0;
      //          v.m_BoneWeights[1] = 0.0f;
      //      }
      //      if (v.m_BoneIDs[2] == -1) {
      //          v.m_BoneIDs[2] = 0;
      //          v.m_BoneWeights[2] = 0.0f;
      //      }
      //      if (v.m_BoneIDs[3] == -1) {
      //          v.m_BoneIDs[3] = 0;
      //          v.m_BoneWeights[3] = 0.0f;
      //      }
      //  }
    }

    SkinnedMesh::~SkinnedMesh()
    {
    }

    void SkinnedMesh::GetFinalTransforms(std::vector<glm::mat4>& Transforms)
    {
        Transforms.resize(10);
        std::fill(Transforms.begin(), Transforms.end(), glm::mat4(1.0));
    }



}
