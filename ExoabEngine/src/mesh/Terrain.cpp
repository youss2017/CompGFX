#include "Terrain.hpp"
#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>

Terrain Terrain_Create(int width, int xresolution, int height, int yresolution, int divide_count)
{
	Terrain terrain;
	width = (width % 2 == 0) ? width : width + 1;
	height = (height % 2 == 0) ? height : height + 1;
	xresolution = std::max(xresolution, 1);
	yresolution = std::max(yresolution, 1);
	terrain.m_width = width;
	terrain.m_height = height;
	using namespace std;
	using namespace glm;
	vector<TerrainVertex> vertices;
	vector<uint32_t> indices;
	// Generate flat terrain
	int indice = 0;
	int number_width = 0;
	int max = (width * xresolution + 1) * (height * yresolution + 1);
	while (max >= 1)
	{
		max /= 10;
		number_width++;
	}
	int step_y = int(1.0 / (float)yresolution);
	int step_x = int(1.0 / (float)xresolution);
	for (int h = -height / 2; h <= height / 2; h++)
	{
		float base_y = h;
		for (int yr = 0; yr < yresolution; yr++)
		{
			float y = base_y + (yr * step_y);
	
			for (int w = -width / 2; w <= width / 2; w++)
			{
				float base_x = w;
				for (int xr = 0; xr < xresolution; xr++)
				{
					float x = base_x + (xr * step_x);
					TerrainVertex vertex;
					vertex.inPosition[0] = x;
					vertex.inPosition[1] = 1.0f;
					vertex.inPosition[2] = y;
					vertex.inNormal[0] = 0;
					vertex.inNormal[1] = -1;
					vertex.inNormal[2] = 0;
					vertex.inTextureIDs[0] = 0;
					vertex.inTextureIDs[1] = -1;
					vertex.inTextureIDs[2] = -1;
					vertex.inTextureWeights[0] = 1.0;
					vertex.inTextureWeights[1] = 0.0;
					vertex.inTextureWeights[2] = 0.0;
					float u = (base_x + (xr * step_x) + ((float)width / 2.0)) / (float)(width);
					float v = (base_y + (yr * step_y) + ((float)height / 2.0)) / (float)(height);
					vertex.inTexCoords[0] = u;
					vertex.inTexCoords[1] = v;
					vertices.push_back(vertex);
				}
			}
		}
	}
	// Generate Indices
	width++;
	width *= xresolution;
	height *= yresolution;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (x == width - 1)
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

	// calculate tangent and bitangent
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
		T0.inTangent = T1.inTangent = T2.inTangent = -1.0f * tangent;
		T0.inBiTangent = T1.inBiTangent = T2.inBiTangent = bitangent;
	}

	terrain.m_vertices = vertices;
	terrain.m_indices = indices;
	terrain.m_totalVerticesCount = terrain.m_vertices.size();
	terrain.m_totalIndicesCount = terrain.m_indices.size();

	return terrain;
}

