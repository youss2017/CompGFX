#pragma once
#include "../mesh/Terrain.hpp"
#include "memory/Texture2.hpp"

// scale down terrain and rendered from top down

namespace Application {

	ITexture2 GenerateMinimap(Terrain* terrain, std::vector<ITexture2> terrainTextures, ITexture2 normalMap);

}
