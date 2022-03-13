#include "EntityController.hpp"
#include <backend/VkGraphicsCard.hpp>

extern vk::VkContext gContext;

EntityController::EntityController(std::vector<Mesh::Geometry>& geometry)
: m_geometry(geometry), m_drawCount(0u)
{
	uint32_t drawCount = 0;
	for (auto& g : geometry)
		drawCount += g.mSubmeshList.size();
	m_drawCount = drawCount;
	m_entSlots.resize(drawCount);
	m_ents.resize(drawCount);
	for (int i = 0; i < gFrameOverlapCount; i++) {
		m_objectBuffer[i] = Buffer2_Create(gContext, BufferType::StorageBuffer, drawCount * sizeof(ShaderTypes::ObjectData), BufferMemoryType::CPU_TO_CPU);
		m_objDataPtr[i] = (ShaderTypes::ObjectData*)Buffer2_Map(m_objectBuffer[i]);
		m_drawBuffer[i] = Buffer2_Create(gContext, BufferType(BufferType::StorageBuffer | BufferType::IndirectBuffer), drawCount * sizeof(ShaderTypes::ObjectData), BufferMemoryType::CPU_TO_CPU);
		m_drawPtr[i] = (ShaderTypes::DrawData*)Buffer2_Map(m_drawBuffer[i]);
	}
	std::fill(m_entSlots.begin(), m_entSlots.end(), true);
	std::fill(m_ents.begin(), m_ents.end(), nullptr);
}
EntityController::~EntityController()
{
	for (int i = 0; i < gFrameOverlapCount; i++) {
		Buffer2_Destroy(m_objectBuffer[i]);
		Buffer2_Destroy(m_drawBuffer[i]);
	}
}

bool EntityController::AddEntity(IEntity entity)
{
	for(uint32_t available_slot = 0; available_slot < m_entSlots.size(); available_slot++) {
		if (m_entSlots[available_slot]) {
			m_entSlots[available_slot] = false;
			m_ents[available_slot] = entity;
			entity->m_reserved_objectdata_index = available_slot;
			m_drawCount++;
			return true;
		}
	}
	return false;
}
void EntityController::RemoveEntity(IEntity entity)
{
	m_entSlots[entity->m_reserved_objectdata_index] = false;
	m_ents[entity->m_reserved_objectdata_index] = nullptr;
	m_drawCount--;
}

void EntityController::Prepare(uint32_t frameIndex)
{
	uint32_t drawCommandIndex = 0;
	for (auto& ent : m_ents) {
		if (!ent)
			continue;
		memcpy(&m_objDataPtr[frameIndex][ent->m_reserved_objectdata_index], &ent->m_objData, sizeof(ShaderTypes::ObjectData));
		Mesh::Geometry& g = m_geometry[ent->m_geometryID];
		for (int i = 0; i < g.mSubmeshList.size(); i++) {
			VkDrawIndexedIndirectCommand command;
			const Mesh::GeometrySubmesh& submesh = g.mSubmeshList[i];
			command.indexCount = submesh.indicesCount;
			command.instanceCount = 1;
			command.firstIndex = submesh.firstIndex;
			command.vertexOffset = submesh.firstVertex;
			command.firstInstance = 0;
			memcpy(&m_drawPtr[frameIndex][drawCommandIndex].command, &command, sizeof(VkDrawIndexedIndirectCommand));
			memcpy(&m_drawPtr[frameIndex][drawCommandIndex].ObjectDataIndex, &ent->m_reserved_objectdata_index, sizeof(uint32_t));
			memcpy(&m_drawPtr[frameIndex][drawCommandIndex].TexIndex, &ent->m_textureID, sizeof(uint32_t));
			drawCommandIndex++;
		}
	}
}