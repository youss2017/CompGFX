#pragma once
#include "../ShaderTypes.hpp"
#include <memory/Buffer2.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace ecs {

	struct Entity;

	struct EntityGeometry
	{
		// The geometryID is used to determine firstVertex/firstIndex/indicesCount
		// The geometryID matches with the index of that geometry in std::vector<Mesh::Geometry>
		int m_geometryID;
		uint32_t mInstanceCount;
		bool* bInstanceSlots;
		std::vector<Entity*> vEnts;
		int nAllocatedInstanceCount;
		// This buffer should be the same size as InstanceBuffer however you do not write to this buffer
		// instead the compute shader culls the instances and writes to this buffer. This instance buffer
		// is what is used in the vertex shader.
		IBuffer2 mCulledInstanceBuffer;
		// Reserved for EntityController
		uint32_t m_reserved_geometrydata_index;
		// Holds the index in the array to VkDraw..IndirectDrawData,
		// for all submeshes
		std::vector<int> m_reserved_drawdata_indices;

		inline uint64_t GetGPUPointer() {
			return nPointer;
		}

		inline uint64_t GetCulledGPUPointer() {
			return nCulledPointer;
		}
		void AddEntity(Entity* e);
		void RemoveEntity(Entity* e);

	private:
		friend void ProcessEntityGeometry(EntityGeometry* EG);
		friend void ECS_ConfigureEntityGeometry(EntityGeometry* eg, int nMaxInstanceCount);
		friend class EntityController;
		IBuffer2 mInstanceBuffer;
		void* pInstanceMapped;
		uint64_t nPointer;
		uint64_t nCulledPointer;
	};

	typedef EntityGeometry* IEntityGeometry;

	void ECS_ConfigureEntityGeometry(EntityGeometry* eg, int nMaxInstanceCount);

	struct Entity {
		Entity() : pEG(nullptr), pSlotIndex(nullptr) {}
		Entity(IEntityGeometry eg) : pEG(eg), pSlotIndex(nullptr) { sData = {}; }
		Entity(IEntityGeometry eg, const ShaderTypes::InstanceData& data) : pEG(eg), pSlotIndex(nullptr) { sData = {data}; }
		IEntityGeometry pEG;
		int* pSlotIndex;
		ShaderTypes::InstanceData sData;
	};

}
