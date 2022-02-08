/*
    Created: 11/26/2021
    Last Modified: 11/27/2021
    Load all mesh data into a struct and provide mesh utilities.
    Load all animation data.
    *** General Info ***
    1) Texture UV coordinates are set to 0, 0 if there are no texture coordinates in model file.
    *** Assumptions ***
    1) We only have 1 UV Channel
    *** TODO ***
    1) Extract Materials from aiScene
    2) Extract Animation Data 
*/
#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_interpolation.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>
#include <assimp/matrix4x4.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/vector3.h>
#include <assimp/matrix4x4.h>
#include <cassert>
#include <vector>
#include <glm/glm.hpp>
#include <string>
#include <map>

struct OmegaBasicVertex {
    glm::fvec3 m_position;
    glm::fvec3 m_normal;
    glm::fvec2 m_uv;

};

struct OmegaBasicSubMesh {
    std::string m_submesh_name;
    std::vector<OmegaBasicVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

struct OmegaBasicMesh {
    std::string m_name;
    std::vector<OmegaBasicSubMesh> m_submesh;
};

OmegaBasicMesh Omega_LoadBasicMesh(const char* mesh_path);

struct OmegaVertex {
    glm::fvec3 m_position;
    glm::fvec3 m_normal;
    glm::fvec3 m_tanget;
    glm::fvec3 m_bitanget;
    glm::fvec2 m_uv;
};

struct OmegaSubMesh {
    std::string m_submesh_name;
    std::vector<OmegaVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

struct OmegaMesh {
    std::string m_name;
    std::vector<OmegaSubMesh> m_submesh;
};

OmegaMesh Omega_LoadMesh(const char* mesh_path);

////////////////////////// ANIMATION STRUCTS
struct OmegaAnimatedVertex {
    glm::fvec3 m_position;
    glm::fvec3 m_normal;
    glm::fvec3 m_tanget;
    glm::fvec3 m_bitanget;
    glm::ivec4 m_boneIDs; // point to a array of transformations
    glm::fvec4 m_boneWeights; // how much each transformation affects the vertex
    glm::fvec2 m_uv;
};

struct OmegaBoneInfo {
    int m_ID;
    /*offset matrix transforms vertex from model space to bone space*/
    glm::mat4 m_offset;
};

struct OmegaAnimatedSubMesh {
    std::string m_submesh_name;
    std::vector<OmegaAnimatedVertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

struct OmegaAnimatedMesh {
    std::string m_name;
    std::map<std::string, OmegaBoneInfo> m_BoneInfoMap;
    int32_t m_BoneCount;
    std::vector<OmegaAnimatedSubMesh> m_submesh;
};

OmegaAnimatedMesh Omega_LoadAnimatedMesh(const char* mesh_path);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class AssimpGLMHelpers
{
public:

	static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
	{
		glm::mat4 to;
		//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
		to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
		to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
		to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
		return to;
	}

	static inline glm::vec3 GetGLMVec(const aiVector3D& vec) 
	{ 
		return glm::vec3(vec.x, vec.y, vec.z); 
	}

	static inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
	{
		return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
	}
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct OmegaKeyPosition {
    glm::vec3 m_position;
    float m_timestamp;
};

struct OmegaKeyRotation {
    glm::quat m_orientation;
    float m_timestamp;
};

struct OmegaKeyScale {
    glm::vec3 m_scale;
    float m_timestamp;
};

class Bone
{
private:
    std::vector<OmegaKeyPosition> m_Positions;
    std::vector<OmegaKeyRotation> m_Rotations;
    std::vector<OmegaKeyScale> m_Scales;
    int m_NumPositions = 0;
    int m_NumRotations = 0;
    int m_NumScalings = 0;
	
    glm::mat4 m_LocalTransform;
    std::string m_Name;
    int m_ID;

public:

/*reads keyframes from aiNodeAnim*/
    Bone(const std::string& name, int ID, const aiNodeAnim* channel)
        :
        m_LocalTransform(1.0f),
        m_Name(name),
        m_ID(ID)
    {
        m_NumPositions = channel->mNumPositionKeys;

        for (int positionIndex = 0; positionIndex < m_NumPositions; ++positionIndex)
        {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            float timeStamp = channel->mPositionKeys[positionIndex].mTime;
            OmegaKeyPosition data;
            data.m_position = AssimpGLMHelpers::GetGLMVec(aiPosition);
            data.m_timestamp = timeStamp;
            m_Positions.push_back(data);
        }

        m_NumRotations = channel->mNumRotationKeys;
        for (int rotationIndex = 0; rotationIndex < m_NumRotations; ++rotationIndex)
        {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
            OmegaKeyRotation data;
            data.m_orientation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
            data.m_timestamp = timeStamp;
            m_Rotations.push_back(data);
        }

        m_NumScalings = channel->mNumScalingKeys;
        for (int keyIndex = 0; keyIndex < m_NumScalings; ++keyIndex)
        {
            aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
            float timeStamp = channel->mScalingKeys[keyIndex].mTime;
            OmegaKeyScale data;
            data.m_scale = AssimpGLMHelpers::GetGLMVec(scale);
            data.m_timestamp = timeStamp;
            m_Scales.push_back(data);
        }
    }
	
    /*interpolates  b/w positions,rotations & scaling keys based on the curren time of 
    the animation and prepares the local transformation matrix by combining all keys 
    tranformations*/
    void Update(float animationTime)
    {
        glm::mat4 translation = InterpolatePosition(animationTime);
        glm::mat4 rotation = InterpolateRotation(animationTime);
        glm::mat4 scale = InterpolateScaling(animationTime);
        m_LocalTransform = translation * rotation * scale;
    }

    glm::mat4 GetLocalTransform() { return m_LocalTransform; }
    std::string GetBoneName() const { return m_Name; }
    int GetBoneID() { return m_ID; }
	

    /* Gets the current index on mKeyPositions to interpolate to based on 
    the current animation time*/
    int GetPositionIndex(float animationTime)
    {
        for (int index = 0; index < m_NumPositions - 1; ++index)
        {
            if (animationTime < m_Positions[index + 1].m_timestamp)
                return index;
        }
        assert(0);
    }

    /* Gets the current index on mKeyRotations to interpolate to based on the 
    current animation time*/
    int GetRotationIndex(float animationTime)
    {
        for (int index = 0; index < m_NumRotations - 1; ++index)
        {
            if (animationTime < m_Rotations[index + 1].m_timestamp)
                return index;
        }
        assert(0);
    }

    /* Gets the current index on mKeyScalings to interpolate to based on the 
    current animation time */
    int GetScaleIndex(float animationTime)
    {
        for (int index = 0; index < m_NumScalings - 1; ++index)
        {
            if (animationTime < m_Scales[index + 1].m_timestamp)
                return index;
        }
        assert(0);
    }

private:

    /* Gets normalized value for Lerp & Slerp*/
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
    {
        float scaleFactor = 0.0f;
        float midWayLength = animationTime - lastTimeStamp;
        float framesDiff = nextTimeStamp - lastTimeStamp;
        scaleFactor = midWayLength / framesDiff;
        return scaleFactor;
    }

    /*figures out which position keys to interpolate b/w and performs the interpolation 
    and returns the translation matrix*/
    glm::mat4 InterpolatePosition(float animationTime)
    {
        if (1 == m_NumPositions)
            return glm::translate(glm::mat4(1.0f), m_Positions[0].m_position);

        int p0Index = GetPositionIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Positions[p0Index].m_timestamp,
            m_Positions[p1Index].m_timestamp, animationTime);
        glm::vec3 finalPosition = glm::mix(m_Positions[p0Index].m_position,
            m_Positions[p1Index].m_position, scaleFactor);
        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    /*figures out which rotations keys to interpolate b/w and performs the interpolation 
    and returns the rotation matrix*/
    glm::mat4 InterpolateRotation(float animationTime)
    {
        if (1 == m_NumRotations)
        {
            auto rotation = glm::normalize(m_Rotations[0].m_orientation);
            return glm::toMat4(rotation);
        }

        int p0Index = GetRotationIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Rotations[p0Index].m_timestamp,
            m_Rotations[p1Index].m_timestamp, animationTime);
        glm::quat finalRotation = glm::slerp(m_Rotations[p0Index].m_orientation,
            m_Rotations[p1Index].m_orientation, scaleFactor);
        finalRotation = glm::normalize(finalRotation);
        return glm::toMat4(finalRotation);
    }

    /*figures out which scaling keys to interpolate b/w and performs the interpolation 
    and returns the scale matrix*/
    glm::mat4 InterpolateScaling(float animationTime)
    {
        if (1 == m_NumScalings)
            return glm::scale(glm::mat4(1.0f), m_Scales[0].m_scale);

        int p0Index = GetScaleIndex(animationTime);
        int p1Index = p0Index + 1;
        float scaleFactor = GetScaleFactor(m_Scales[p0Index].m_timestamp,
            m_Scales[p1Index].m_timestamp, animationTime);
        glm::vec3 finalScale = glm::mix(m_Scales[p0Index].m_scale, m_Scales[p1Index].m_scale
            , scaleFactor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }
	
};

struct AssimpNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

class Animation
{
public:
    Animation() = default;

    Animation(const std::string& animationPath, OmegaAnimatedMesh* model)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
        assert(scene && scene->mRootNode);
        auto animation = scene->mAnimations[0];
        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond;
        ReadHeirarchyData(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, *model);
    }

    ~Animation()
    {
    }

    Bone* FindBone(const std::string& name)
    {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
            [&](const Bone& Bone)
            {
                return Bone.GetBoneName() == name;
            }
        );
        if (iter == m_Bones.end()) return nullptr;
        else return &(*iter);
    }

	
    inline float GetTicksPerSecond() { return m_TicksPerSecond; }

    inline float GetDuration() { return m_Duration;}

    inline const AssimpNodeData& GetRootNode() { return m_RootNode; }

    inline const std::map<std::string, OmegaBoneInfo>& GetBoneIDMap() 
    { 
        return m_BoneInfoMap;
    }

private:
    void ReadMissingBones(const aiAnimation* animation, OmegaAnimatedMesh& model)
    {
        int size = animation->mNumChannels;

        auto& boneInfoMap = model.m_BoneInfoMap;//getting m_BoneInfoMap from Model class
        int& boneCount = model.m_BoneCount; //getting the m_BoneCounter from Model class

        //reading channels(bones engaged in an animation and their keyframes)
        for (int i = 0; i < size; i++)
        {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;

            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                boneInfoMap[boneName].m_ID = boneCount;
                boneCount++;
            }
            m_Bones.push_back(Bone(channel->mNodeName.data,
                boneInfoMap[channel->mNodeName.data].m_ID, channel));
        }

        m_BoneInfoMap = boneInfoMap;
    }

    void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
    {
        assert(src);

        dest.name = src->mName.data;
        dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = src->mNumChildren;

        for (uint32_t i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            ReadHeirarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }
    float m_Duration;
    int m_TicksPerSecond;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::map<std::string, OmegaBoneInfo> m_BoneInfoMap;
};

class Animator
{	
public:
    Animator(Animation* Animation)
    {
        m_CurrentTime = 0.0;
        m_CurrentAnimation = Animation;

        m_FinalBoneMatrices.reserve(100);

        for (int i = 0; i < 100; i++)
            m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
    }
	
    void UpdateAnimation(float dt)
    {
        m_DeltaTime = dt;
        if (m_CurrentAnimation)
        {
            m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
            m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
            CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
        }
    }
	
    void PlayAnimation(Animation* pAnimation)
    {
        m_CurrentAnimation = pAnimation;
        m_CurrentTime = 0.0f;
    }
	
    void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
    {
        std::string nodeName = node->name;
        glm::mat4 nodeTransform = node->transformation;
	
        Bone* Bone = m_CurrentAnimation->FindBone(nodeName);
	
        if (Bone)
        {
            Bone->Update(m_CurrentTime);
            nodeTransform = Bone->GetLocalTransform();
        }
	
        glm::mat4 globalTransformation = parentTransform * nodeTransform;
	
        auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
        if (boneInfoMap.find(nodeName) != boneInfoMap.end())
        {
            int index = boneInfoMap[nodeName].m_ID;
            glm::mat4 offset = boneInfoMap[nodeName].m_offset;
            m_FinalBoneMatrices[index] = globalTransformation * offset;
        }
	
        for (int i = 0; i < node->childrenCount; i++)
            CalculateBoneTransform(&node->children[i], globalTransformation);
    }
	
    std::vector<glm::mat4> GetFinalBoneMatrices() 
    { 
        return m_FinalBoneMatrices;  
    }
		
private:
    std::vector<glm::mat4> m_FinalBoneMatrices;
    Animation* m_CurrentAnimation;
    float m_CurrentTime;
    float m_DeltaTime;	
};
