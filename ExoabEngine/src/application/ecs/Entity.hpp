#pragma once
#include "../ShaderTypes.hpp"
#include <memory/Buffer2.hpp>
#include <glm/glm.hpp>
#include <vector>

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
	// The geometryID is used to determine firstVertex/firstIndex/indicesCount
	// The geometryID matches with the index of that geometry in std::vector<Mesh::Geometry>
	EntityGeometryID m_geometryID;
	uint32_t mInstanceCount;
	IBuffer2 mInstanceBuffer;
	// This buffer should be the same size as InstanceBuffer however you do not write to this buffer
	// instead the compute shader culls the instances and writes to this buffer. This instance buffer
	// is what is used in the vertex shader.
	IBuffer2 mCulledInstanceBuffer;
	// Reserved for EntityController
	uint32_t m_reserved_geometrydata_index;
	std::vector<int> m_reserved_drawdata_indices;
};

typedef Entity* IEntity;
#define NewEntity() (new Entity)
