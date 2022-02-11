#include "Graphics.hpp"
#include "CommandDefines.hpp"
#include "../utils/MBox.hpp"
#include "../utils/common.hpp"
#include "../utils/StringUtils.hpp"
#include "material_system/Material.hpp"
#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

#include "../units/Entity.hpp"

static void GL_Graphics3D_Internal_UploadShaderData(GLuint programID, IShaderProgramData reflection, EntityBindingData *data);
static void GL_Graphics3D_Internal_BindTextures(GLuint programID, IShaderProgramData reflection);

void GL_Graphics3D_ExecuteCommandList(IGraphics3D gfx, ICommandList CmdList, IGPUFence WaitFence, uint32_t WaitSemaphoreCount, IGPUSemaphore *pWaitSemaphores, uint32_t SignalSemaphoreCount, IGPUSemaphore *pSignalSemaphores)
{
    PROFILE_SCOPE("Graphics3D::ExecuteCommandList()");
    gfx->m_OpenGLLock.lock();
    for (int i = 0; i < CmdList->m_GLCmd.size(); i++)
    {
        GLCommandData &command = CmdList->m_GLCmd[i];
        // switch statements are utter garabage
        if (command.m_CmdID == COMMAND_SWITCH_MATERIAL)
        {
            IPipelineState pipeline = nullptr;
            IFramebuffer fbo = nullptr;
            Material *material = (Material *)command.material;
            pipeline = material->m_pipeline_state;
            fbo = material->m_framebuffer->m_framebuffer;
            glViewport(0, 0, fbo->m_width, fbo->m_height);
            glScissor(0, 0, fbo->m_width, fbo->m_height);
            glBindFramebuffer(GL_FRAMEBUFFER, PVOIDToUInt32(fbo->m_framebuffer));

            // Process Framebuffer According to Users request
            // TODO: Set Blend State
            IFramebufferStateManagement state_managment = pipeline->m_StateManagment;
            GLenum draw_buffers[31];
            for (uint32_t i = 0; i < state_managment->m_ColorAttachmentCount; i++)
            {
                draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
                glDrawBuffer(draw_buffers[i]);
                auto &att = state_managment->m_attachments[i];
                // glColorMask(i, att.BlendState.colorWriteMask & VK_COLOR_COMPONENT_R_BIT,
                //                 att.BlendState.colorWriteMask & VK_COLOR_COMPONENT_G_BIT,
                //                 att.BlendState.colorWriteMask & VK_COLOR_COMPONENT_B_BIT,
                //                 att.BlendState.colorWriteMask & VK_COLOR_COMPONENT_A_BIT);
                if (att.m_loading == VK_ATTACHMENT_LOAD_OP_CLEAR)
                {
                    glClearColor(att.clearColor.color.float32[0], att.clearColor.color.float32[1], att.clearColor.color.float32[2], att.clearColor.color.float32[3]);
                    glClear(GL_COLOR_BUFFER_BIT);
                }
            }
            glDrawBuffers(state_managment->m_ColorAttachmentCount, draw_buffers);

            auto specification = pipeline->m_spec;
            if (state_managment->m_depth_attachment.has_value())
            {
                auto att = state_managment->m_depth_attachment.value();
                if (att.m_loading == VK_ATTACHMENT_LOAD_OP_CLEAR)
                {
                    glClearDepth(specification.m_FarField);
                    glClear(GL_DEPTH_BUFFER_BIT);
                }
            }

            if (!gfx->m_PipelineInitalized)
            {
                gfx->m_PipelineInitalized = true;
                gfx->m_PipelineSpecification = specification;
                auto spec = gfx->m_PipelineSpecification._Storage;
                if (spec.m_CullMode == CullMode::CULL_BACK)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
                else if (spec.m_CullMode == CullMode::CULL_FRONT)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                }
                else if (spec.m_CullMode == CullMode::CULL_FRONT_AND_BACK)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT_AND_BACK);
                }
                else if (spec.m_CullMode == CullMode::CULL_NONE)
                {
                    glDisable(GL_CULL_FACE);
                }
                if (spec.m_DepthEnabled)
                    glEnable(GL_DEPTH_TEST);
                else
                    glDisable(GL_DEPTH_TEST);
                if (spec.m_DepthFunc == DepthFunction::ALWAYS)
                    glDepthFunc(GL_ALWAYS);
                else if (spec.m_DepthFunc == DepthFunction::EQUAL)
                    glDepthFunc(GL_EQUAL);
                else if (spec.m_DepthFunc == DepthFunction::GREATER)
                    glDepthFunc(GL_GREATER);
                else if (spec.m_DepthFunc == DepthFunction::GREATER_EQUAL)
                    glDepthFunc(GL_GEQUAL);
                else if (spec.m_DepthFunc == DepthFunction::LESS)
                    glDepthFunc(GL_LESS);
                else if (spec.m_DepthFunc == DepthFunction::LESS_EQUAL)
                    glDepthFunc(GL_LEQUAL);
                else if (spec.m_DepthFunc == DepthFunction::NEVER)
                    glDepthFunc(GL_NEVER);
                else if (spec.m_DepthFunc == DepthFunction::NOT_EQUAL)
                    glDepthFunc(GL_NOTEQUAL);
                glDepthRange(spec.m_NearField, spec.m_FarField);
                glDepthMask(spec.m_DepthWriteEnable);
                glFrontFace(spec.m_FrontFaceCCW ? GL_CW : GL_CCW);
                if (spec.m_PolygonMode == PolygonMode::FILL)
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                else if (spec.m_PolygonMode == PolygonMode::LINE)
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                else if (spec.m_PolygonMode == PolygonMode::POINT)
                    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                if (spec.m_SampleRateShadingEnabled)
                    glEnable(GL_SAMPLE_SHADING);
                else
                    glDisable(GL_SAMPLE_SHADING);
                if (glMinSampleShading)
                    glMinSampleShading(spec.m_MinSampleShading);
            }
            else
            {
                auto current_spec = gfx->m_PipelineSpecification._Storage;
                auto new_spec = specification;
                auto spec = specification;
                if (spec.m_CullMode == CullMode::CULL_BACK)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
                else if (spec.m_CullMode == CullMode::CULL_FRONT)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                }
                else if (spec.m_CullMode == CullMode::CULL_FRONT_AND_BACK)
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT_AND_BACK);
                }
                else if (spec.m_CullMode == CullMode::CULL_NONE)
                {
                    glDisable(GL_CULL_FACE);
                }
                if (spec.m_DepthEnabled)
                    glEnable(GL_DEPTH_TEST);
                else
                    glDisable(GL_DEPTH_TEST);
                if (spec.m_DepthFunc == DepthFunction::ALWAYS)
                    glDepthFunc(GL_ALWAYS);
                else if (spec.m_DepthFunc == DepthFunction::EQUAL)
                    glDepthFunc(GL_EQUAL);
                else if (spec.m_DepthFunc == DepthFunction::GREATER)
                    glDepthFunc(GL_GREATER);
                else if (spec.m_DepthFunc == DepthFunction::GREATER_EQUAL)
                    glDepthFunc(GL_GEQUAL);
                else if (spec.m_DepthFunc == DepthFunction::LESS)
                    glDepthFunc(GL_LESS);
                else if (spec.m_DepthFunc == DepthFunction::LESS_EQUAL)
                    glDepthFunc(GL_LEQUAL);
                else if (spec.m_DepthFunc == DepthFunction::NEVER)
                    glDepthFunc(GL_NEVER);
                else if (spec.m_DepthFunc == DepthFunction::NOT_EQUAL)
                    glDepthFunc(GL_NOTEQUAL);
                if (current_spec.m_NearField != new_spec.m_NearField || current_spec.m_FarField != new_spec.m_FarField)
                    glDepthRange(spec.m_NearField, spec.m_FarField);
                if (current_spec.m_DepthWriteEnable != new_spec.m_DepthWriteEnable)
                    glDepthMask(spec.m_DepthWriteEnable);
                //if (current_spec.m_FrontFaceCCW != new_spec.m_FrontFaceCCW)
                //    ;
                glFrontFace(spec.m_FrontFaceCCW ? GL_CW : GL_CCW);
                if (current_spec.m_PolygonMode != new_spec.m_PolygonMode)
                {
                    if (spec.m_PolygonMode == PolygonMode::FILL)
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    else if (spec.m_PolygonMode == PolygonMode::LINE)
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    else if (spec.m_PolygonMode == PolygonMode::POINT)
                        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                }
                if (current_spec.m_SampleRateShadingEnabled != new_spec.m_SampleRateShadingEnabled)
                {
                    if (spec.m_SampleRateShadingEnabled)
                        glEnable(GL_SAMPLE_SHADING);
                    else
                        glDisable(GL_SAMPLE_SHADING);
                }
                if (current_spec.m_MinSampleShading != new_spec.m_MinSampleShading)
                    if(glMinSampleShading)
                        glMinSampleShading(spec.m_MinSampleShading);
                gfx->m_PipelineSpecification = specification;
            }
            glUseProgram(PVOIDToUInt32(pipeline->m_pipeline));
        }
        else if (command.m_CmdID == COMMAND_STOP_SHADER_PROGRAM)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glUseProgram(0);
        }
        else if (command.m_CmdID == COMMAND_SET_VIEWPORT)
        {
            glViewport(command.x, command.y, command.width, command.height);
        }
        else if (command.m_CmdID == COMMAND_SET_SCISSOR)
        {
            glScissor(command.x, command.y, command.width, command.height);
        }
        else if (command.m_CmdID == COMMAND_DRAW_ARRAYS)
        {
            auto topolgy = gfx->m_PipelineSpecification._Storage.m_Topology;
            GLenum gltopology;
            if (topolgy == PolygonTopology::LINE_LIST)
            {
                gltopology = GL_LINES;
            }
            else if (topolgy == PolygonTopology::POINT_LIST)
            {
                gltopology = GL_POINTS;
            }
            else
            {
                gltopology = GL_TRIANGLES;
            }
            glDrawArraysInstanced(gltopology, command.vertices_offset, command.vertices_count, command.instance_count);
        }
        else if (command.m_CmdID == COMMAND_DRAW_INDEXED)
        {
            auto topolgy = gfx->m_PipelineSpecification._Storage.m_Topology;
            GLenum gltopology;
            if (topolgy == PolygonTopology::LINE_LIST)
            {
                gltopology = GL_LINES;
            }
            else if (topolgy == PolygonTopology::POINT_LIST)
            {
                gltopology = GL_POINTS;
            }
            else
            {
                gltopology = GL_TRIANGLES;
            }
            glDrawElementsInstancedBaseVertex(gltopology, command.indices_count, GL_UNSIGNED_INT, nullptr, command.instance_count, command.vertices_offset);
        }
        else if (command.m_CmdID == COMMAND_SET_RENDERSTATE)
        {
            glBindVertexArray(command.render_state.m_VaoID);
        }
        else if (command.m_CmdID == COMMAND_SET_PUSHCONSTANT)
        {
            GLuint programID = PVOIDToUInt32(command.pipeline_state->m_pipeline);
            GLint location = glGetUniformLocation(programID, command.name);
            if (location == -1)
            {
                logerror_break("Could not find pushconstant location (OpenGL)");
            }
            // TODO: Switch to uniform blocks
            assert(0);
        }
        else if (command.m_CmdID == COMMAND_DRAW_DEFAULT_ENTITIES)
        {
            std::vector<DefaultEntity>& entities = command.entities;
            Material *material = (Material *)command.material;
            GLuint programID = PVOIDToUInt32(material->m_pipeline_state->m_pipeline);
            for (int i = 0; i < entities.size(); i++)
            {
                DefaultEntity& entity = entities[i];
                // 1) Set Render State (Bind VAO)
                auto& renderState = entity.m_model->GetRenderState();
                // 2) Update Instancing Date if it changed
                glBindVertexArray(renderState.m_VaoID);
                if(entity.m_InstanceBufferUpdated)
                {
                    entity.m_InstanceBufferUpdated = false;
                    PipelineVertexInputDescription& input_description =  material->m_pipeline_layout->input_description;
                    int CurrentBinding = 0, bufferID = 0, index = 0;
                    for(auto& element : input_description.m_input_elements)
                    {
                        if(!element.m_per_instance) {
                            index++;
                            continue;
                        }
                        if(CurrentBinding != element.m_binding_id)
                        {
                            glBindBuffer(GL_ARRAY_BUFFER, entity.VertexInstanceBuffers[bufferID]->m_NativeOpenGLHandle);
                            bufferID++;
                        }
                        CurrentBinding = element.m_binding_id;
                        if(element.m_IsFloatingPoint)
                            glVertexAttribPointer(index, element.m_vector_size, GL_FLOAT, GL_FALSE, element.m_stride, UInt32ToPVOID(element.m_offset));
                        else {
                            glVertexAttribIPointer(index, element.m_vector_size, element.m_IsUnsigned ? GL_UNSIGNED_INT : GL_INT, element.m_stride, UInt32ToPVOID(element.m_offset));
                        }
                        glVertexAttribDivisor(index, 1);
                        glEnableVertexAttribArray(index);
                        index++;
                    }
                }
                // 2) Upload Shader Data
                for (int j = 0; j < entity.size(); j++)
                {
                    auto &data = entity[j];
                    GL_Graphics3D_Internal_UploadShaderData(programID, command.program_data, &data);
                }
                // 3) Bind Textures
                GL_Graphics3D_Internal_BindTextures(programID, command.program_data);
                // 4) Issue Draw Commands
                if (renderState.m_IndicesCount > 0)
                    glDrawElementsInstanced(GL_TRIANGLES, renderState.m_IndicesCount, GL_UNSIGNED_INT, nullptr, entity.DrawInstanceCount);
                else
                    glDrawArraysInstanced(GL_TRIANGLES, 0, renderState.m_VerticesCount, entity.DrawInstanceCount);
            }
        }
        else
        {
            Utils::Message("OpenGL Command Executation Error.", "Graphics3D Encourtered an unknown OpenGL command, this is cause by unimplemented command, memory corruption, or an invalid call.", Utils::MBox::ERR);
            Utils::Break(); // Unknown OpenGL Command or Not implemented!
        }
    }
    glBindVertexArray(0);
    gfx->m_OpenGLLock.unlock();
}

// TODO: Optimize glGetUniformLocation
static void GL_Graphics3D_Internal_UploadShaderData(GLuint programID, IShaderProgramData reflection, EntityBindingData *bindingData)
{
    // 1) Find Buffer, Does it exist?
    int SetIDMissing;
    auto buffer = reflection->m_combined_reflection.GetBufferDescription(bindingData->setID, bindingData->bindingID, &SetIDMissing);
    if (SetIDMissing)
    {
        std::stringstream ss;
        ss << "Could not find SetID '" << bindingData->setID << "', and bindingID '" << bindingData->bindingID << "'";
        logerrors_break(ss.str());
        return;
    }
    else if (!buffer)
    {
        std::stringstream ss;
        ss << "Could not find Binding '" << bindingData->bindingID << ", in setID '" << bindingData->setID << "'";
        logerrors_break(ss.str());
        return;
    }
    for (int i = 0; i < bindingData->size(); i++)
    {
        const EntityBindingData::BindingData &data = (*bindingData)[i];
        std::string access_name = buffer->m_Name + "." + data.name;
        GLint location = glGetUniformLocation(programID, access_name.c_str());
        if (location == -1)
        {
            // could not find location
            std::stringstream ss;
            ss << "(OpenGL Error) Could not find uniform location for setID '" << bindingData->setID << "', and bindingID '" << bindingData->bindingID << "' while using access name '" << access_name << "'";
            logerrors_break(ss.str());
        }
        else
        {
            if (data.type == ENTITY_DATA_TYPE_MAT4)
            {
                glUniformMatrix4fv(location, 1, GL_FALSE, (GLfloat *)&data.dataunion.matrix4);
            }
            else if (data.type == ENTITY_DATA_TYPE_MAT3)
            {
                glUniformMatrix3fv(location, 1, GL_FALSE, (GLfloat *)&data.dataunion.matrix3);
            }
            else if (data.type == ENTITY_DATA_TYPE_MAT2)
            {
                glUniformMatrix2fv(location, 1, GL_FALSE, (GLfloat *)&data.dataunion.matrix2);
            }
            else if (data.type == ENTITY_DATA_TYPE_FLOAT)
            {
                glUniform1f(location, data.dataunion.float32);
            }
            else if (data.type == ENTITY_DATA_TYPE_INT)
            {
                glUniform1i(location, data.dataunion.int32);
            }
            else if (data.type == ENTITY_DATA_TYPE_UINT)
            {
                glUniform1ui(location, data.dataunion.uint32);
            }
            else
            {
                logerror("Unimplemented Flag for EntityDataType, fix me.");
                Utils::Break();
            }
        }
    }

    return;

}

static void GL_Graphics3D_Internal_BindTextures(GLuint programID, IShaderProgramData reflection)
{
    // 1) Find Image Array, Does it exist?
    int SetIDMissing;
    if (reflection->m_textures.size() == 0)
        return;
    auto texture = reflection->m_combined_reflection.GetSampledImageDescription(reflection->m_texture_setID, reflection->m_texture_bindingID, &SetIDMissing);
    if (SetIDMissing)
    {
        std::stringstream ss;
        ss << "[Texture] Could not find SetID '" << reflection->m_texture_setID << "', and bindingID '" << reflection->m_texture_bindingID << "'";
        logerrors_break(ss.str());
        return;
    }
#pragma message ("Optimize me! GLGraphics3D.cpp Line 358")
    for (int i = 0; i < reflection->m_textures.size(); i++)
    {
        std::string access_name = texture->m_Name + "[" + std::to_string(i) + "]";
        GLint location = glGetUniformLocation(programID, access_name.c_str());
        // This may cause problems.
        glUniform1i(location, i);
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, PVOIDToUInt32(reflection->m_textures[i]->m_NativeHandle));
    }
}