#version 450 core
#include "types.h"

layout (std140, set = 0, binding = 0) readonly buffer VerticesSSBO {
    AnimatedVertex u_Vertices[];
};

layout (std140, set = 0, binding = 1) uniform MVP {
    mat4 u_Model;
    mat4 u_View;
    mat4 u_Projection;
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

layout (std140, set = 0, binding = 2) uniform BoneMatrices {
    mat4 u_FinalBoneMatrices[MAX_BONES];
};

void main() {

}