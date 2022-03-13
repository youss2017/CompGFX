/*
        Copyright 2011 Etay Meiri
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SkinnedMesh.hpp"
#include <assimp/postprocess.h>
#include <iostream>

namespace Mesh {

    using namespace std;

    SkinnedMesh::SkinnedMesh(const string& Filename)
    {
        bool Ret = false;
        Assimp::Importer imp;
        pScene = imp.ReadFile(Filename.c_str(), aiProcess_PopulateArmatureData | aiProcessPreset_TargetRealtime_MaxQuality);
        imp.GetOrphanedScene();
        if (pScene) {
            Ret = InitFromScene(pScene, Filename);
        }
        else {
            char elog[100];
            elog[0] = 0;
            sprintf(elog, "Error parsing '%s': '%s'\n", Filename.c_str(), imp.GetErrorString());
            throw std::runtime_error(elog);
        }
    }

    SkinnedMesh::~SkinnedMesh()
    {
    }

    bool SkinnedMesh::InitFromScene(const aiScene* pScene, const string& Filename)
    {
        m_Meshes.resize(pScene->mNumMeshes);

        unsigned int NumVertices = 0;
        unsigned int NumIndices = 0;

        CountVerticesAndIndices(pScene, NumVertices, NumIndices);

        ReserveSpace(NumVertices, NumIndices);

        InitAllMeshes(pScene);

        if (!InitMaterials(pScene, Filename)) {
            return false;
        }

        return true;
    }


    void SkinnedMesh::CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices)
    {
        for (unsigned int i = 0; i < m_Meshes.size(); i++) {
            m_Meshes[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
            m_Meshes[i].BaseVertex = NumVertices;
            m_Meshes[i].BaseIndex = NumIndices;

            NumVertices += pScene->mMeshes[i]->mNumVertices;
            NumIndices += m_Meshes[i].NumIndices;
        }
    }


    void SkinnedMesh::ReserveSpace(unsigned int NumVertices, unsigned int NumIndices)
    {
        mVertices.reserve(NumVertices);
        mIndices.reserve(NumIndices);
        m_Bones.resize(NumVertices);
    }


    void SkinnedMesh::InitAllMeshes(const aiScene* pScene)
    {
        for (unsigned int i = 0; i < m_Meshes.size(); i++) {
            const aiMesh* paiMesh = pScene->mMeshes[i];
            InitSingleMesh(i, paiMesh);
        }
    }


    void SkinnedMesh::InitSingleMesh(uint32_t MeshIndex, const aiMesh* paiMesh)
    {
        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

        // Populate the vertex attribute vectors
        for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
            VertexBone v;
            const aiVector3D& pPos = paiMesh->mVertices[i];
            //m_Positions.push_back(glm::vec3(pPos.x, pPos.y, pPos.z));
            v.m_Positions = glm::vec4(pPos.x, pPos.y, pPos.z, 1.0);

            const aiVector3D& pNormal = paiMesh->mNormals[i];
            //m_Normals.push_back(glm::vec3(pNormal.x, pNormal.y, pNormal.z));
            v.m_Normals = glm::vec4(pNormal.x, pNormal.y, pNormal.z, 0.0);

            const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : Zero3D;
            //m_TexCoords.push_back(glm::vec2(pTexCoord.x, pTexCoord.y));
            v.m_TexCoords = glm::vec4(pTexCoord.x, pTexCoord.y, 0.0f, 0.0f);
            v.m_BoneWeights = glm::vec4(0.0);
            v.m_BoneIDs = glm::ivec4(-1);
            mVertices.push_back(v);
        }

        LoadMeshBones(MeshIndex, paiMesh);

        // Populate the index buffer
        for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
            const aiFace& Face = paiMesh->mFaces[i];
            //        printf("num indices %d\n", Face.mNumIndices);
            //        assert(Face.mNumIndices == 3);
            mIndices.push_back(Face.mIndices[0]);
            mIndices.push_back(Face.mIndices[1]);
            mIndices.push_back(Face.mIndices[2]);
        }

        // load bone info into vertices
        for (unsigned int i = 0; i < paiMesh->mNumBones; i++) {
            aiBone* bone = paiMesh->mBones[i];
            aiVertexWeight* weight = bone->mWeights;
            VertexBone& v = mVertices[weight->mVertexId];
            if (v.m_BoneIDs[0] == -1) {
                v.m_BoneIDs[0] = i;
                v.m_BoneWeights[0] = weight->mWeight;
            } else if (v.m_BoneIDs[1] == -1) {
                v.m_BoneIDs[1] = i;
                v.m_BoneWeights[1] = weight->mWeight;
            } else if (v.m_BoneIDs[2] == -1) {
                v.m_BoneIDs[2] = i;
                v.m_BoneWeights[2] = weight->mWeight;
            } else if (v.m_BoneIDs[3] == -1) {
                v.m_BoneIDs[3] = i;
                v.m_BoneWeights[3] = weight->mWeight;
            }
            else {
                assert(0);
            }
        }

    }

    void SkinnedMesh::LoadMeshBones(uint32_t MeshIndex, const aiMesh* pMesh)
    {
        for (uint32_t i = 0; i < pMesh->mNumBones; i++) {
            LoadSingleBone(MeshIndex, pMesh->mBones[i]);
        }
    }

    inline glm::mat4 GlmMatrix(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    void SkinnedMesh::LoadSingleBone(uint32_t MeshIndex, const aiBone* pBone)
    {
        int BoneId = GetBoneId(pBone);

        if (BoneId == m_BoneInfo.size()) {
            BoneInfo bi(GlmMatrix(pBone->mOffsetMatrix));
            m_BoneInfo.push_back(bi);
        }

        for (uint32_t i = 0; i < pBone->mNumWeights; i++) {
            const aiVertexWeight& vw = pBone->mWeights[i];
            uint32_t GlobalVertexID = m_Meshes[MeshIndex].BaseVertex + pBone->mWeights[i].mVertexId;
            m_Bones[GlobalVertexID].AddBoneData(BoneId, vw.mWeight);
        }
    }


    int SkinnedMesh::GetBoneId(const aiBone* pBone)
    {
        int BoneIndex = 0;
        string BoneName(pBone->mName.C_Str());
        
        if (m_BoneNameToIndexMap.find(BoneName) == m_BoneNameToIndexMap.end()) {
            // Allocate an index for a new bone
            BoneIndex = (int)m_BoneNameToIndexMap.size();
            m_BoneNameToIndexMap[BoneName] = BoneIndex;
        }
        else {
            BoneIndex = m_BoneNameToIndexMap[BoneName];
        }

        return BoneIndex;
    }


    string GetDirFromFilename(const string& Filename)
    {
        // Extract the directory part from the file name
        string::size_type SlashIndex;

#ifdef _WIN64
        SlashIndex = Filename.find_last_of("\\");

        if (SlashIndex == -1) {
            SlashIndex = Filename.find_last_of("/");
        }
#else
        SlashIndex = Filename.find_last_of("/");
#endif

        string Dir;

        if (SlashIndex == string::npos) {
            Dir = ".";
        }
        else if (SlashIndex == 0) {
            Dir = "/";
        }
        else {
            Dir = Filename.substr(0, SlashIndex);
        }

        return Dir;
    }


    bool SkinnedMesh::InitMaterials(const aiScene* pScene, const string& Filename)
    {
        return true;
    }

    void SkinnedMesh::GetBoneTransforms(vector<glm::mat4>& Transforms)
    {
        Transforms.resize(m_BoneInfo.size());

        glm::mat4 Identity = glm::mat4(1.0);

        ReadNodeHierarchy(pScene->mRootNode, Identity);

        for (uint32_t i = 0; i < m_BoneInfo.size(); i++) {
            Transforms[i] = m_BoneInfo[i].FinalTransformation;
        }
    }


    void SkinnedMesh::ReadNodeHierarchy(const aiNode* pNode, const glm::mat4& ParentTransform)
    {
        string NodeName(pNode->mName.data);

        glm::mat4 NodeTransformation(GlmMatrix(pNode->mTransformation));

        //printf("%s - ", NodeName.c_str());

        glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

        if (m_BoneNameToIndexMap.find(NodeName) != m_BoneNameToIndexMap.end()) {
            uint32_t BoneIndex = m_BoneNameToIndexMap[NodeName];
            m_BoneInfo[BoneIndex].FinalTransformation = GlobalTransformation * m_BoneInfo[BoneIndex].OffsetMatrix;
        }

        for (uint32_t i = 0; i < pNode->mNumChildren; i++) {
            ReadNodeHierarchy(pNode->mChildren[i], GlobalTransformation);
        }
    }

}
