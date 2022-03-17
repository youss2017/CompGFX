#pragma once
#include "Entity.hpp"
#include "../../mesh/geometry.hpp"
#include <memory/Buffer2.hpp>
#include <map>

class EntityController {

public:
	EntityController(const std::vector<Mesh::Geometry>& geometry);
	~EntityController();
	IEntity GetEntity(EntityGeometryID id) {
#if defined(_DEBUG)
		if (mEntites.find(id) == mEntites.end()) {
			logerror("Could not find EntityGeometryID!");
			Utils::Break();
		}
#endif
		return mEntites[id];
	}
	void PrepareDataForFrame(uint32_t frameIndex);
	uint32_t GetInstanceCount();
	inline uint32_t GetDrawCount() { return mDraws.size(); }
	inline IBuffer2* GetDrawDataArray() { return mInputDrawData; }
	inline IBuffer2* GetGeometryDataArray() { return mInputGeometryData; }
private:
	std::vector<Mesh::Geometry> mGeometry;
	std::vector<ShaderTypes::DrawData> mDraws;
	std::vector<ShaderTypes::GeometryData> mGeoData;
	std::map<EntityGeometryID, IEntity> mEntites;
	IBuffer2 mInputDrawData[gFrameOverlapCount];
	IBuffer2 mInputGeometryData[gFrameOverlapCount];
};