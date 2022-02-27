#include "EntityController.hpp"

void EntityController::AddEntity(Entity* entity)
{
	entity->m_reserved_objectdata_index = m_next_objdata_index++;
	m_entites.push_back(entity);
}

void EntityController::UpdateDrawCommandAndObjectDataBuffer(ShaderTypes::DrawData* drawCommands, ShaderTypes::ObjectData* objectData)
{
	int i = 0;
	for (auto& e : m_entites)
	{
		const Mesh::Geometry& geometry = m_geometry[e->m_geometryID];
		ShaderTypes::DrawData drawCommand;
		drawCommand.command.firstIndex = geometry.firstIndex;
		drawCommand.command.firstInstance = 0;
		drawCommand.command.indexCount = geometry.indicesCount;
		drawCommand.command.instanceCount = 1;
		drawCommand.command.vertexOffset = geometry.firstVertex;
		drawCommand.ObjectDataIndex = e->m_reserved_objectdata_index;
		drawCommand.TexIndex = e->m_textureID;
		memcpy(&drawCommands[i], &drawCommand, sizeof(ShaderTypes::DrawData));
		memcpy(&objectData[i], &e->m_objData, sizeof(ShaderTypes::ObjectData));
		i++;
	}
}

void EntityController::UpdateDrawCommandAndObjectDataBufferSection(uint32_t offset, uint32_t size, ShaderTypes::DrawData* drawCommands, ShaderTypes::ObjectData* objectData)
{
	assert(offset + size <= m_maxDrawCount);
	for (int i = offset; i < m_entites.size(); i++)
	{
		Entity* e = m_entites[i];
		const Mesh::Geometry& geometry = m_geometry[e->m_geometryID];
		ShaderTypes::DrawData drawCommand;
		drawCommand.command.firstIndex = geometry.firstIndex;
		drawCommand.command.firstInstance = 0;
		drawCommand.command.indexCount = geometry.indicesCount;
		drawCommand.command.instanceCount = 1;
		drawCommand.command.vertexOffset = geometry.firstVertex;
		drawCommand.ObjectDataIndex = e->m_reserved_objectdata_index;
		drawCommand.TexIndex = e->m_textureID;
		memcpy(&drawCommands[i], &drawCommand, sizeof(ShaderTypes::DrawData));
		memcpy(&objectData[i], &e->m_objData, sizeof(ShaderTypes::ObjectData));
		i++;
	}
}
