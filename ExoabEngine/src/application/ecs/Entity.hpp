#pragma once
#include "../ShaderTypes.hpp"
#include <memory/Buffer2.hpp>
#include <glm/glm.hpp>
#include <vector>

struct Entity
{
	// The geometryID is used to determine firstVertex/firstIndex/indicesCount
	// The geometryID matches with the index of that geometry in std::vector<Mesh::Geometry>
	int m_geometryID;
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
