#include "Entity.hpp"
#include "Globals.hpp"

namespace ecs {

	void ProcessEntityGeometry(EntityGeometry* EG)
	{
		for (int i = 0; i < EG->nAllocatedInstanceCount; i++) {
			Entity* e = EG->vEnts[i];
			memcpy((char*)EG->pInstanceMapped + (i * sizeof(ShaderTypes::InstanceData)), &e->sData, sizeof(ShaderTypes::InstanceData));
		}
		Buffer2_Flush(EG->mInstanceBuffer, 0, sizeof(ShaderTypes::InstanceData) * EG->nAllocatedInstanceCount);
	}

	void ECS_ConfigureEntityGeometry(EntityGeometry* eg, int nMaxInstanceCount)
	{
		eg->bInstanceSlots = new bool[nMaxInstanceCount];
		memset(eg->bInstanceSlots, INT_MAX, sizeof(bool) * nMaxInstanceCount);
		eg->mInstanceCount = nMaxInstanceCount;
		eg->mInstanceBuffer = Buffer2_CreatePreInitalized(Global::Context, BufferType::BUFFER_TYPE_STORAGE, nullptr, sizeof(ShaderTypes::InstanceData) * nMaxInstanceCount, BufferMemoryType::CPU_TO_GPU, true, false);
		eg->mCulledInstanceBuffer = Buffer2_CreatePreInitalized(Global::Context, BufferType::BUFFER_TYPE_STORAGE, nullptr, sizeof(ShaderTypes::InstanceData) * nMaxInstanceCount, BufferMemoryType::CPU_TO_GPU, true, false);
		eg->vEnts.resize(nMaxInstanceCount);
		eg->nAllocatedInstanceCount = 0;
		eg->pInstanceMapped = Buffer2_Map(eg->mInstanceBuffer);
		eg->nPointer = Buffer2_GetGPUPointer(eg->mInstanceBuffer);
		eg->nCulledPointer = Buffer2_GetGPUPointer(eg->mCulledInstanceBuffer);
	}

	void EntityGeometry::AddEntity(Entity* e) {
		assert(nAllocatedInstanceCount <= (int)mInstanceCount && "Cannot not add entity because instance count is too small.");
		for (int i = 0; i < (int)mInstanceCount; i++) {
			if (bInstanceSlots[i]) {
				bInstanceSlots[i] = false;
				e->pSlotIndex = new int{ i };
				vEnts[i] = e;
				nAllocatedInstanceCount++;
				break;
			}
		}
	}

	void EntityGeometry::RemoveEntity(Entity* e) {
		int slotIndex = *e->pSlotIndex;
		if (slotIndex == -1) {
			logerror("Invalid Remove Entity!");
		}
		delete e->pSlotIndex;
		// Move the last entity to the current entity position to be deleted.
		// ex ====> [ENT][ENT][ENT][ENT][ENT][...][...][...]...
		//						|
		// ex ====> [ENT][ENT][...][ENT][ENT(N)][...][...][...]...
		// ex ====> [ENT][ENT][ENT(N)][ENT][...][...][...][...]...
		if (nAllocatedInstanceCount > 1) {
			Entity* EntN = vEnts[nAllocatedInstanceCount - 1];
			bInstanceSlots[nAllocatedInstanceCount - 1] = true;
			vEnts[slotIndex] = EntN;
			*EntN->pSlotIndex = slotIndex;
		}
		nAllocatedInstanceCount--;
	}

}
