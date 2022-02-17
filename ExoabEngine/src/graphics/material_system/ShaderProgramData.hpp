#pragma once
#include "../../core/shaders/ShaderReflection.hpp"
#include "../../core/memory/Textures.hpp"
#include "../../core/pipeline/Pipeline.hpp"
#include "../../units/Entity.hpp"

struct SSBOInformation
{
	IBuffer2 m_ssbobuffer;
	uint32_t m_setID, m_bindingID;
};

// One Per Thread
struct ShaderProgramData
{
	GraphicsContext m_context;
	ShaderReflection m_vertex_reflection;
	ShaderReflection m_fragment_reflection;
	ShaderReflection m_combined_reflection;
	IGPUTextureSampler m_sampler = nullptr;
	std::vector<IGPUTexture2D> m_textures;
	std::vector<SSBOInformation> m_ssbos;
	bool m_textures_updated;
	int m_texture_setID;
	int m_texture_bindingID;
	// A pointer allocated for vulkan to store buffers/data/objects needed to upload shader data
	void* m_reserved;
};

typedef ShaderProgramData* IShaderProgramData;

// One Per Thread, One Per Material
IShaderProgramData ShaderProgramData_Create(IPipelineLayout layout);
// TODO: Support multiple texture arrays.
// When changing constant textures, the Engine stalls until GPU completes all work and then updates the textures.
void ShaderProgramData_SetConstantTextureArray(IShaderProgramData programData, int setID, int bindingID, IGPUTextureSampler sampler, const std::vector<IGPUTexture2D>& textures);
void ShaderProgramData_SetConstantSSBO(IShaderProgramData programData, int setID, int bindingID, IBuffer2 ssbo_buffer);
void ShaderProgramData_UpdateBindingData(IShaderProgramData programData, IPipelineLayout layout, uint32_t count, EntityBindingData* bindingData);
void ShaderProgramData_UpdateEntityBindingData(IShaderProgramData programData, IPipelineLayout layout, uint32_t count, void** entities);
void ShaderProgramData_FlushShaderProgramData(uint32_t count, IShaderProgramData* pShaderDatas);
void ShaderProgramData_Destroy(IShaderProgramData programData);

//void* ShaderProgramData_UpdateData(IShaderProgramData programData, std::vector<EntityBindingData> s);
