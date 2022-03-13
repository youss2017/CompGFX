/*
    Copyright 2021 Etay Meiri
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

#pragma once

#include <map>
#include <vector>
#include <string>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <glm/glm.hpp>

namespace Mesh {

    struct VertexBone {
        glm::vec4 m_Positions;
        glm::vec4 m_Normals;
        glm::vec4 m_TexCoords;
        glm::vec4 m_BoneWeights;
        glm::ivec4 m_BoneIDs;
    };

    class SkinnedMesh
    {
    public:
        SkinnedMesh(const std::string& Filename);
        ~SkinnedMesh();

        uint32_t NumBones() const
        {
            return (uint32_t)m_BoneNameToIndexMap.size();
        }

        void GetBoneTransforms(std::vector<glm::mat4>& Transforms);

    private:

        bool InitFromScene(const aiScene* pScene, const std::string& Filename);

        void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);

        void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

        void InitAllMeshes(const aiScene* pScene);

        void InitSingleMesh(uint32_t MeshIndex, const aiMesh* paiMesh);

        bool InitMaterials(const aiScene* pScene, const std::string& Filename);

        struct VertexBoneData
        {
            uint32_t BoneIDs[4] = { 0 };
            float Weights[4] = { 0.0f };

            VertexBoneData()
            {
            }

            void AddBoneData(uint32_t BoneID, float Weight)
            {
                for (uint32_t i = 0; i < /* ARRAY_SIZE_IN_ELEMENTS(BoneIDs) */ 4; i++) {
                    if (Weights[i] == 0.0) {
                        BoneIDs[i] = BoneID;
                        Weights[i] = Weight;
                        //printf("Adding bone %d weight %f at index %i\n", BoneID, Weight, i);
                        return;
                    }
                }

                // should never get here - more bones than we have space for
                assert(0);
            }
        };

        void LoadMeshBones(uint32_t MeshIndex, const aiMesh* paiMesh);
        void LoadSingleBone(uint32_t MeshIndex, const aiBone* pBone);
        int GetBoneId(const aiBone* pBone);
        void ReadNodeHierarchy(const aiNode* pNode, const glm::mat4& ParentTransform);

        struct BasicMeshEntry {
            BasicMeshEntry()
            {
                NumIndices = 0;
                BaseVertex = 0;
                BaseIndex = 0;
            }

            unsigned int NumIndices;
            unsigned int BaseVertex;
            unsigned int BaseIndex;
        };

        const aiScene* pScene = NULL;
        std::vector<BasicMeshEntry> m_Meshes;

    public:
        std::vector<VertexBone> mVertices;
        std::vector<unsigned int> mIndices;
    private:
        std::vector<VertexBoneData> m_Bones;

        std::map<std::string, uint32_t> m_BoneNameToIndexMap;

        struct BoneInfo
        {
            glm::mat4 OffsetMatrix;
            glm::mat4 FinalTransformation;

            BoneInfo(const glm::mat4& Offset)
            {
                OffsetMatrix = Offset;
                FinalTransformation = glm::mat4(0.0);
            }
        };

        std::vector<BoneInfo> m_BoneInfo;
    };
}
