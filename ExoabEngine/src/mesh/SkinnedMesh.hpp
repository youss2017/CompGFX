#pragma once

#include <map>
#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>     
#include <glm/glm.hpp>

namespace Mesh {

    struct BoneInfo
    {
        glm::mat4 OffsetMatrix;
        glm::mat4 InverseOffsetMatrix;
        glm::mat4 FinalTransformation;

        BoneInfo(const glm::mat4& Offset)
        {
            OffsetMatrix = Offset;
            InverseOffsetMatrix = glm::inverse(Offset);
            FinalTransformation = FinalTransformation * Offset;
        }
    };

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
        void GetFinalTransforms(std::vector<glm::mat4>& Transforms);

    public:
        std::vector<VertexBone> mVertices;
        std::vector<unsigned int> mIndices;
    private:
        std::vector<BoneInfo> mBoneInfo;
        const aiScene* mScene;
    };
}
