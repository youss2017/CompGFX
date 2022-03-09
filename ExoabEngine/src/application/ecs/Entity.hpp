#pragma once
#include "../ShaderTypes.hpp"
#include <glm/glm.hpp>

// TODO: Replace these with real mesh (not testing meshes)
enum EntityGeometryID : int
{
	ENTITY_GEOMETRY_CUBE = 0,
	ENTITY_GEOMETRY_BALL = 1
};

struct BoundingBox {
	glm::vec3 minimum_point;
	glm::vec3 maximum_point;
};

struct Entity
{
	EntityGeometryID m_geometryID;
	ShaderTypes::ObjectData m_objData;
	uint32_t m_textureID;
	BoundingBox m_box;
	glm::vec3 m_velocity;
	glm::vec3 m_acceleration;
	
	// Reserved for EntityController
	uint32_t m_reserved_objectdata_index;
};

typedef Entity* IEntity;
#define NewEntity() (new Entity)
