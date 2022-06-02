#pragma once
#include "Entity.hpp"
#include "../../mesh/geometry.hpp"
#include <memory/Buffer2.hpp>
#include <map>

namespace ecs {

	class EntityController {

	public:
		EntityController(const std::vector<Mesh::Geometry>& geometry);
		~EntityController();
		IEntityGeometry GetEntity(int id) {
#if defined(_DEBUG)
			if (mEntites.find(id) == mEntites.end()) {
				logerror("Could not find EntityGeometryID! Did you include the //# Path in the config file?");
				Utils::Break();
			}
#endif
			return mEntites[id];
		}
		void PrepareDataForFrame();
		inline uint32_t GetDrawCount() { return mDraws.size(); }
		inline IBuffer2 GetDrawDataBuffer() { return mInputDrawData; }
		inline IBuffer2 GetGeometryDataBuffer() { return mInputGeometryData; }
		uint32_t GetInstanceCount();
	private:
		std::vector<Mesh::Geometry> mGeometry;
		std::vector<ShaderTypes::DrawData> mDraws;
		std::vector<ShaderTypes::GeometryData> mGeoData;
		std::map<int, IEntityGeometry> mEntites;
		IBuffer2 mInputDrawData;
		IBuffer2 mInputGeometryData;
	};
}
