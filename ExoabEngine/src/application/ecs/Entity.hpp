#pragma once
#include "../ShaderTypes.hpp"
#include <memory/Buffer2.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace ecs {

	class Entity;

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

	private:
		friend ecs::Entity;
		friend void ProcessEntityGeometry(EntityGeometry* EG);
		void AddEntity(Entity* e);
		void RemoveEntity(Entity* e);
	private:
		friend class EntityController;
		friend void ECS_ConfigureEntityGeometry(EntityGeometry* eg, int nMaxInstanceCount);
		IBuffer2 mInstanceBuffer;
		void* pInstanceMapped;
		uint64_t nPointer;
		uint64_t nCulledPointer;
	};

	typedef EntityGeometry* IEntityGeometry;

	void ECS_ConfigureEntityGeometry(EntityGeometry* eg, int nMaxInstanceCount);

	class Entity {

	public:
		Entity(IEntityGeometry eg);
		~Entity();
		Entity(const Entity& e) = delete;
		Entity(const Entity&& e) = delete;

		IEntityGeometry pEG;
		int* pSlotIndex;
		ShaderTypes::InstanceData sData;
	};

}
