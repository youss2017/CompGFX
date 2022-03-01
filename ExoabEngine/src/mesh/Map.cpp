#include "Map.hpp"
#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>

Map Map_Create(int width, int xresolution, int height, int yresolution)
{
	Map terrain;
	width = (width % 2 == 0) ? width : width + 1;
	height = (height % 2 == 0) ? height : height + 1;
	xresolution = std::max(xresolution, 1);
	yresolution = std::max(yresolution, 1);
	terrain.m_width = width;
	terrain.m_height = height;
	using namespace std;
	using namespace glm;
	vector<MapVertex> vertices;
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
	for (int h = -height / 2; h <= height / 2; h++)
	{
		float base_y = h;
		float step_y = 1.0 / (float)yresolution;
		for (int yr = 0; yr < yresolution; yr++)
		{
			float y = base_y + (yr * step_y);
			//cout << y << ": ";

			for (int w = -width / 2; w <= width / 2; w++)
			{
				float base_x = w;
				float step_x = 1.0 / (float)xresolution;
				for (int xr = 0; xr < xresolution; xr++)
				{
					float x = base_x + (xr * step_x);
					MapVertex vertex;
					vertex.inPosition[0] = x;
					vertex.inPosition[1] = 0;
					vertex.inPosition[2] = y;
					vertex.inPosition[3] = 1.0;
					vertex.inNormal[0] = 0;
					vertex.inNormal[1] = 1.0;
					vertex.inNormal[2] = 0;
					vertex.inTextureIDs[0] = 0;
					vertex.inTextureIDs[1] = 0;
					vertex.inTextureIDs[2] = 0;
					vertex.inTextureIDs[3] = 0;
					vertex.inTextureWeights[0] = 1.0;
					vertex.inTextureWeights[1] = 0.0;
					vertex.inTextureWeights[2] = 0.0;
					vertex.inTextureWeights[3] = 0.0;
					float u = (base_x + (xr * step_x) + ((float)width / 2.0)) / (float)(width);
					float v = (base_y + (yr * step_y) + ((float)height / 2.0)) / (float)(height);
					vertex.inTexCoords[0] = u;
					vertex.inTexCoords[1] = v;
					vertices.push_back(vertex);
					//cout << "(" << u << "," << v << ") ";
					//cout << indice++ << " ";
					//printf("%*i ", number_width, indice++);
				}
			}
			//cout << "\n";
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
			//printf("(%*i, %*i, %*i) ", number_width, indexA, number_width, indexB, number_width, indexC);
			//printf("(%*i, %*i, %*i) ", number_width, indexB, number_width, indexD, number_width, indexC);
		}
		//cout << endl;
	}
	terrain.m_vertices = vertices;
	terrain.m_indices = indices;

	// Break up terrain
	uint32_t meshFragmentCount = 4;
	uint32_t submeshTriangleCount = (indices.size() / 3) / meshFragmentCount;
	vector<vector<MapVertex>> split_mesh;
	vector<MapVertex> temp_vertices;
	for (int i = 0, j = 0; i < indices.size(); i++) {
		if (j == submeshTriangleCount * 3) {
			j = 0;
			split_mesh.push_back(temp_vertices);
			temp_vertices.clear();
		}
		temp_vertices.push_back(vertices[indices[i]]);
		j++;
	}
	return terrain;
}

