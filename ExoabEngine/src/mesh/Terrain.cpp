#include "Terrain.hpp"
#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>

static void Terrain_CalculateTangents(TerrainInfo* terrain) {
	auto& vertices = terrain->m_vertices;
	auto& indices = terrain->m_indices;
	for (int i = 0; i < indices.size() / 3;) {
		auto& T0 = vertices[indices[i++]];
		auto& T1 = vertices[indices[i++]];
		auto& T2 = vertices[indices[i++]];
		auto deltaPos1 = T1.inPosition - T0.inPosition;
		auto deltaPos2 = T2.inPosition - T0.inPosition;
		auto deltaUV1 = T1.inTexCoords - T0.inTexCoords;
		auto deltaUV2 = T2.inTexCoords - T0.inTexCoords;
		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		auto tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent;
		bitangent.x = r * (-deltaUV2.x * deltaPos1.x + deltaUV1.x * deltaPos2.x);
		bitangent.y = r * (-deltaUV2.x * deltaPos1.y + deltaUV1.x * deltaPos2.y);
		bitangent.z = r * (-deltaUV2.x * deltaPos1.z + deltaUV1.x * deltaPos2.z);
		tangent = glm::normalize(tangent);
		bitangent = glm::normalize(bitangent);
		T0.inNormal = T1.inNormal = T2.inNormal = glm::cross(deltaPos1, deltaPos2);
		T0.inTangent = T1.inTangent = T2.inTangent = -1.0f * tangent;
		T0.inBiTangent = T1.inBiTangent = T2.inBiTangent = bitangent;
	}
}

TerrainInfo Terrain_Create(int width, int height)
{
	TerrainInfo terrain;
	width = (width % 2 == 0) ? width : width + 1;
	height = (height % 2 == 0) ? height : height + 1;
	terrain.m_width = width;
	terrain.m_height = height;
	using namespace std;
	using namespace glm;
	vector<TerrainVertex> vertices;
	vector<uint32_t> indices;
	// Generate flat terrain
	for (int h = 0; h < height; h++)
	{
		for (int w = 0; w < width; w++)
		{
				TerrainVertex vertex;
				vertex.inPosition[0] = w;
				vertex.inPosition[1] = 1.0f;
				vertex.inPosition[2] = h;
				vertex.inNormal[0] = 0;
				vertex.inNormal[1] = -1;
				vertex.inNormal[2] = 0;
				vertex.inTextureIDs[0] = 0;
				vertex.inTextureIDs[1] = -1;
				vertex.inTextureIDs[2] = -1;
				vertex.inTextureWeights[0] = 1.0;
				vertex.inTextureWeights[1] = 0.0;
				vertex.inTextureWeights[2] = 0.0;
				float u = (w + ((float)width / 2.0)) / (float)(width);
				float v = (h + ((float)height / 2.0)) / (float)(height);
				vertex.inTexCoords[0] = u;
				vertex.inTexCoords[1] = v;
				vertices.push_back(vertex);
		}
	}
	// Generate Indices
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (x == (width - 1))
				continue;
			uint32_t indexA = y * width + x;
			uint32_t indexB = indexA + 1;
			uint32_t indexC = (y + 1) * width + x;
			uint32_t indexD = indexC + 1;
			indices.push_back(indexA);
			indices.push_back(indexB);
			indices.push_back(indexC);

			indices.push_back(indexB);
			indices.push_back(indexD);
			indices.push_back(indexC);
		}
	}

	terrain.m_vertices = vertices;
	terrain.m_indices = indices;
	terrain.m_totalVerticesCount = terrain.m_vertices.size();
	terrain.m_totalIndicesCount = terrain.m_indices.size();
	terrain.m_width = width;
	terrain.m_height = height;

	Terrain_CalculateTangents(&terrain);

	return terrain;
}

void Terrain_ApplyHeightMap(TerrainInfo* terrain, int MapWidth, int MapHeight, float minHeight, float maxHeight, uint8_t* heightmap)
{
	using namespace glm;
	auto& vertices = terrain->m_vertices;
		
	for (int y = 0; y < terrain->m_height; y++) {
		float yratio = y / float(terrain->m_height);
		for (int x = 0; x < terrain->m_width; x++) {
			float xratio = x / float(terrain->m_width);
			vertices[y * terrain->m_width + x].inPosition.y = glm::clamp(maxHeight * float(heightmap[int((yratio * MapHeight) * MapWidth + (xratio * MapWidth))]) / 255.0f, minHeight, maxHeight);
		}
	}
	Terrain_CalculateTangents(terrain);
}

