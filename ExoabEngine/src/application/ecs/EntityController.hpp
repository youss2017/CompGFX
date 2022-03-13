#pragma once
#include "Entity.hpp"
#include "../../mesh/geometry.hpp"
#include <memory/Buffer2.hpp>

class EntityController {

public:
	EntityController(std::vector<Mesh::Geometry>& geometry);
	~EntityController();

	bool AddEntity(IEntity entitiy);
	void RemoveEntity(IEntity entity);

	void Prepare(uint32_t frameIndex);
	IBuffer2 GetObjectBuffer(uint32_t frameIndex) { return m_objectBuffer[frameIndex]; }
	auto GetObjectBufferArray() { return &m_objectBuffer[0]; }

	IBuffer2 GetDrawBuffer(uint32_t frameIndex) { return m_drawBuffer[frameIndex]; }
	auto GetDrawBufferArray() { return &m_drawBuffer[0]; }
	inline uint32_t GetDrawCount() { return m_drawCount; }

private:
	std::vector<bool> m_entSlots;
	std::vector<IEntity> m_ents;
	uint32_t m_drawCount;
	IBuffer2 m_objectBuffer[gFrameOverlapCount];
	IBuffer2 m_drawBuffer[gFrameOverlapCount];
	ShaderTypes::ObjectData* m_objDataPtr[gFrameOverlapCount];
	ShaderTypes::DrawData* m_drawPtr[gFrameOverlapCount];
	std::vector<Mesh::Geometry> m_geometry;
};