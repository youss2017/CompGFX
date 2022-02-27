#pragma once
#include "Entity.hpp"
#include "../../mesh/geometry.hpp"

class EntityController
{
public:
	EntityController(std::vector<Mesh::Geometry>& geometry, uint32_t MaxDrawCount, uint32_t MaxObjectCount) : m_geometry(geometry), m_maxDrawCount(MaxDrawCount), m_maxObjectCount(MaxObjectCount) {}

	// We need a pointer, so that if you update object data in the entity
	// we can access it in the entity controller.
	void AddEntity(Entity* entity);
	void UpdateDrawCommandAndObjectDataBuffer(ShaderTypes::DrawData* drawCommands, ShaderTypes::ObjectData* objectData);
	void UpdateDrawCommandAndObjectDataBufferSection(uint32_t offset, uint32_t size, ShaderTypes::DrawData* drawCommands, ShaderTypes::ObjectData* objectData);
	inline uint32_t GetDrawCount() { return m_entites.size(); }

private:
	std::vector<Mesh::Geometry> m_geometry;
	std::vector<Entity*> m_entites;
	uint32_t m_maxDrawCount, m_maxObjectCount;
	uint32_t m_next_objdata_index = 0;
};
