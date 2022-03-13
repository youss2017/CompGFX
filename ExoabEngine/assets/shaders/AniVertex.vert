#version 450 core
#extension GL_ARB_shader_draw_parameters : require

#define MAX_BONES 100
#define UseOriginal 1

struct Vertex {
    vec4 position;
    vec4 normal;
    vec4 texcoord;
    vec4 boneWeights;
    ivec4 boneIDs;
};

struct ObjectDataBONE {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 finalBoneTransformations[MAX_BONES];
};

struct DrawCommand {
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    int     vertexOffset;
    uint    firstInstance;
    // my data
    uint objDataIndex;
};

layout (std140, set = 0, binding = 0) readonly buffer VerticesSSBO {
    Vertex u_Vertices[];
};

layout (std140, set = 0, binding = 1) readonly buffer AniObjectData {
    ObjectDataBONE u_objData[];
};

layout (std140, set = 0, binding = 2) readonly buffer DrawCommandSSBO {
    DrawCommand u_draws[];
};

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec2 TexCoord;
layout (location = 2) out vec3 Weights;

#if UseOriginal == 1
#define draw u_draws[gl_DrawIDARB]
#define obj u_objData[draw.objDataIndex]
#define v u_Vertices[gl_VertexIndex]
#endif

void main() {
#if UseOriginal == 0
    DrawCommand cmd = u_draws[gl_DrawIDARB];
    ObjectDataBONE obj = u_objData[cmd.objDataIndex];
    Vertex v = u_Vertices[gl_VertexIndex];
#endif

    mat4 boneTransform = obj.finalBoneTransformations[v.boneIDs[0]] * v.boneWeights[0];
    boneTransform += obj.finalBoneTransformations[v.boneIDs[1]] * v.boneWeights[1];
    boneTransform += obj.finalBoneTransformations[v.boneIDs[2]] * v.boneWeights[2];
    boneTransform += obj.finalBoneTransformations[v.boneIDs[3]] * v.boneWeights[3];
    boneTransform = mat4(1.0);
    vec4 totalPosition = boneTransform * v.position;

    Normal = (boneTransform * vec4(v.normal.xyz, 0.0)).xyz;
    TexCoord = v.texcoord.xy;
    gl_Position = obj.projection * obj.view * obj.model * totalPosition;
    Weights = vec3(v.boneWeights.rgb);
}