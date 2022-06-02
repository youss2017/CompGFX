#include "EntityController.hpp"
#include <backend/VkGraphicsCard.hpp>
#include "Globals.hpp"

namespace ecs {
	EntityController::EntityController(const std::vector<Mesh::Geometry>& geometry)
		: mGeometry(geometry)
	{
		int inputGeometryDataSize = 0;
		for (int drawDataIndex = 0; inputGeometryDataSize < geometry.size(); inputGeometryDataSize++) {
			IEntityGeometry entity = new EntityGeometry{};
			const Mesh::Geometry& g = geometry[inputGeometryDataSize];
			for (auto& submesh : g.mSubmeshList) {
				ShaderTypes::DrawData draw;
				draw.indexCount = submesh.indicesCount;
				draw.instanceCount = 0;
				draw.firstIndex = submesh.firstIndex;
				draw.vertexOffset = submesh.firstVertex;
				draw.firstInstance = 0;
				draw.GeometryDataIndex = inputGeometryDataSize;
				mDraws.push_back(draw);
				entity->m_reserved_drawdata_indices.push_back(drawDataIndex++);
			}
			entity->bInstanceSlots = nullptr;
			entity->m_geometryID = inputGeometryDataSize;
			entity->m_reserved_geometrydata_index = inputGeometryDataSize;
			mEntites.insert(std::make_pair(inputGeometryDataSize, entity));
			ShaderTypes::GeometryData geoData;
			geoData.bounding_sphere_center = g.m_bounding_sphere_center;
			geoData.bounding_sphere_radius = g.m_bounding_sphere_radius;
			geoData.mInstancePtr = 0;
			mGeoData.push_back(geoData);
		}

		mInputDrawData = Buffer2_CreatePreInitalized(BufferType(BUFFER_TYPE_STORAGE | BUFFER_TYPE_TRANSFER_DST | BUFFER_TYPE_INDIRECT), mDraws.data(), mDraws.size() * sizeof(ShaderTypes::DrawData), BufferMemoryType::CPU_TO_GPU, true, true);
		mInputGeometryData = Buffer2_CreatePreInitalized(BufferType(BUFFER_TYPE_STORAGE | BUFFER_TYPE_TRANSFER_DST | BUFFER_TYPE_INDIRECT), mGeoData.data(), mGeoData.size() * sizeof(ShaderTypes::GeometryData), BufferMemoryType::CPU_TO_GPU, true, true);
	}

	EntityController::~EntityController()
	{
		Buffer2_Destroy(mInputDrawData);
		Buffer2_Destroy(mInputGeometryData);
		for (auto& [index, eg] : mEntites) {
			if (eg->mInstanceBuffer)
				Buffer2_Destroy(eg->mInstanceBuffer);
			if (eg->mCulledInstanceBuffer)
				Buffer2_Destroy(eg->mCulledInstanceBuffer);
			if (eg->bInstanceSlots)
				delete[] eg->bInstanceSlots;
			delete eg;
		}
	}

	void ProcessEntityGeometry(EntityGeometry* EG);

	void EntityController::PrepareDataForFrame()
	{
		ShaderTypes::DrawData* draws = (ShaderTypes::DrawData*)Buffer2_Map(mInputDrawData);
		ShaderTypes::GeometryData* geoData = (ShaderTypes::GeometryData*)Buffer2_Map(mInputGeometryData);

		for (auto& entitymap : mEntites) {
			IEntityGeometry eg = entitymap.second;
			Mesh::Geometry g = mGeometry[eg->m_geometryID];
			int sindex = 0;
			ProcessEntityGeometry(eg);
			for (auto drawDataIndex : eg->m_reserved_drawdata_indices) {
				ShaderTypes::DrawData draw;
				const Mesh::GeometrySubmesh& submesh = g.mSubmeshList[sindex];
				draw.indexCount = submesh.indicesCount;
				draw.instanceCount = eg->nAllocatedInstanceCount;
				draw.firstIndex = submesh.firstIndex;
				draw.vertexOffset = submesh.firstVertex;
				draw.firstInstance = 0;
				draw.GeometryDataIndex = eg->m_reserved_geometrydata_index;
				memcpy(draws + drawDataIndex, &draw, sizeof(ShaderTypes::DrawData));
			}
			if (eg->GetGPUPointer()) {
				uint64_t GPUpointer = eg->GetGPUPointer();
				uint64_t CulledGPUpointer = eg->GetCulledGPUPointer();
				memcpy(&(geoData + eg->m_reserved_geometrydata_index)->mInstancePtr, &GPUpointer, 8);
				memcpy(&(geoData + eg->m_reserved_geometrydata_index)->mCulledInstancePtr, &CulledGPUpointer, 8);
			}
		}
		Buffer2_Flush(mInputDrawData, 0, VK_WHOLE_SIZE);
		Buffer2_Flush(mInputGeometryData, 0, VK_WHOLE_SIZE);
	}

	uint32_t EntityController::GetInstanceCount()
	{
		uint32_t c = 0;
		for (auto& eg : this->mEntites) {
			c += eg.second->nAllocatedInstanceCount;
		}
		return c;
	}


}