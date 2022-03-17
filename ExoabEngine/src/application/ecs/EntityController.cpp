#include "EntityController.hpp"
#include <backend/VkGraphicsCard.hpp>

extern vk::VkContext gContext;

EntityController::EntityController(const std::vector<Mesh::Geometry>& geometry)
: mGeometry(geometry)
{
	int inputGeometryDataSize = 0;
	for (int drawDataIndex = 0; inputGeometryDataSize < geometry.size(); inputGeometryDataSize++) {
		IEntity entity = NewEntity();
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
		entity->m_reserved_geometrydata_index = inputGeometryDataSize;
		mEntites.insert(std::make_pair(EntityGeometryID(inputGeometryDataSize), entity));
		ShaderTypes::GeometryData geoData;
		geoData.bounding_sphere_center = g.m_bounding_sphere_center;
		geoData.bounding_sphere_radius = g.m_bounding_sphere_radius;
		geoData.mInstancePtr = 0;
		mGeoData.push_back(geoData);
	}

	for (int i = 0; i < gFrameOverlapCount; i++) {
		mInputDrawData[i] = Buffer2_CreatePreInitalized(gContext, BufferType(BUFFER_TYPE_STORAGE | BUFER_TYPE_TRANSFER_DST), mDraws.data(), mDraws.size() * sizeof(ShaderTypes::DrawData), BufferMemoryType::CPU_TO_CPU, true);
		mInputGeometryData[i] = Buffer2_CreatePreInitalized(gContext, BufferType(BUFFER_TYPE_STORAGE | BUFER_TYPE_TRANSFER_DST), mGeoData.data(), mGeoData.size() * sizeof(ShaderTypes::GeometryData), BufferMemoryType::CPU_TO_CPU, true);
	}
}

EntityController::~EntityController()
{
	for (int i = 0; i < gFrameOverlapCount; i++) {
		Buffer2_Destroy(mInputDrawData[i]);
		Buffer2_Destroy(mInputGeometryData[i]);
	}
}

void EntityController::PrepareDataForFrame(uint32_t frameIndex)
{
	ShaderTypes::DrawData* draws = (ShaderTypes::DrawData*)Buffer2_Map(mInputDrawData[frameIndex]);
	ShaderTypes::GeometryData* geoData = (ShaderTypes::GeometryData*)Buffer2_Map(mInputGeometryData[frameIndex]);

	for (auto& entitymap : mEntites) {
		IEntity entity = entitymap.second;
		Mesh::Geometry g = mGeometry[entity->m_geometryID];
		int sindex = 0;
		for (auto drawDataIndex : entity->m_reserved_drawdata_indices) {
			ShaderTypes::DrawData draw;
			const Mesh::GeometrySubmesh& submesh = g.mSubmeshList[sindex];
			draw.indexCount = submesh.indicesCount;
			draw.instanceCount = entity->mInstanceCount;
			draw.firstIndex = submesh.firstIndex;
			draw.vertexOffset = submesh.firstVertex;
			draw.firstInstance = 0;
			draw.GeometryDataIndex = entity->m_reserved_geometrydata_index;
			memcpy(draws + drawDataIndex, &draw, sizeof(ShaderTypes::DrawData));
		}
		if (entity->mInstanceBuffer) {
			uint64_t GPUpointer = Buffer2_GetGPUPointer(entity->mInstanceBuffer);
			uint64_t CulledGPUpointer = Buffer2_GetGPUPointer(entity->mCulledInstanceBuffer);
			memcpy(&(geoData + entity->m_reserved_geometrydata_index)->mInstancePtr, &GPUpointer, 8);
			memcpy(&(geoData + entity->m_reserved_geometrydata_index)->mCulledInstancePtr, &CulledGPUpointer, 8);
		}
	}
	Buffer2_Flush(mInputDrawData[frameIndex], 0, VK_WHOLE_SIZE);
	Buffer2_Flush(mInputGeometryData[frameIndex], 0, VK_WHOLE_SIZE);
}

uint32_t EntityController::GetInstanceCount()
{
	uint32_t count = 0;
	for (auto e : mEntites)
		count += e.second->mInstanceCount;
	return count;
}
