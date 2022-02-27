#pragma once
#include "../ShaderTypes.hpp"

// TODO: Replace these with real mesh (not testing meshes)

enum EntityGeometryID : int
{
	ENTITY_GEOMETRY_CUBE = 0,
	ENTITY_GEOMETRY_BALL = 1
};

struct Entity
{
	EntityGeometryID m_geometryID;
	ShaderTypes::ObjectData m_objData;
	uint32_t m_textureID;
	// Reserved for EntityController
	uint32_t m_reserved_objectdata_index;
};
