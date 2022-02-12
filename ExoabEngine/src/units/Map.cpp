#include "Map.hpp"
#include <iostream>

ITerrain Terrain_Create(IPipelineLayout layout, int width, int xresolution, int height, int yresolution, IGPUTextureSampler sampler, IGPUTexture2D basetexture, int setID, int bindingID)
{
	ITerrain terrain = new Terrain();
	width = (width % 2 == 0) ? width : width + 1;
	height = (height % 2 == 0) ? height : height + 1;
	xresolution = std::max(xresolution, 1);
	yresolution = std::max(yresolution, 1);
	terrain->m_context = basetexture->m_Context;
	terrain->m_width = width;
	terrain->m_height = height;
	terrain->m_base_texture = basetexture;
	using namespace std;
	using namespace glm;
	// Generate flat terrain
	// Massive memory optimization could happen here.
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
					TerrainVertex vertex;
					vertex.inPosition = vec3(x, 0, y);
					vertex.inNormal = vec3(0, 1, 0);
					vertex.inTextureIDs = uvec3(0);
					vertex.inTextureWeights = vec3(1.0, 0, 0);
					float u = (base_x + (xr * step_x) + ((float)width / 2.0)) / (float)(width);
					float v = (base_y + (yr * step_y) + ((float)height / 2.0)) / (float)(height);
					vertex.inTexCoords = vec2(u, v);
					terrain->m_vertices.push_back(vertex);
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
			terrain->m_indices.push_back(indexA);
			terrain->m_indices.push_back(indexB);
			terrain->m_indices.push_back(indexC);

			terrain->m_indices.push_back(indexB);
			terrain->m_indices.push_back(indexD);
			terrain->m_indices.push_back(indexC);
			//printf("(%*i, %*i, %*i) ", number_width, indexA, number_width, indexB, number_width, indexC);
			//printf("(%*i, %*i, %*i) ", number_width, indexB, number_width, indexD, number_width, indexC);
		}
		//cout << endl;
	}

	terrain->m_vertices_buffer = Buffer2_Create(terrain->m_context, BufferType::VertexBuffer, terrain->m_vertices.size() * sizeof(TerrainVertex), BufferMemoryType::STATIC);
	terrain->m_indices_buffer = Buffer2_Create(terrain->m_context, BufferType::IndexBuffer, terrain->m_indices.size() * sizeof(uint32_t), BufferMemoryType::STATIC);

	Buffer2_UploadData(terrain->m_vertices_buffer, (char8_t*)terrain->m_vertices.data(), 0, terrain->m_vertices.size() * sizeof(TerrainVertex));
	Buffer2_UploadData(terrain->m_indices_buffer, (char8_t*)terrain->m_indices.data(), 0, terrain->m_indices.size() * sizeof(uint32_t));
	
	terrain->m_program_data = ShaderProgramData_Create(layout);
	ShaderProgramData_SetConstantTextureArray(terrain->m_program_data, setID, bindingID, sampler, { basetexture });

	terrain->m_render_state = RenderState::Create(terrain->m_vertices_buffer, terrain->m_indices_buffer, terrain->m_vertices.size(), terrain->m_indices.size());

	terrain->m_binding_data.AddElement("view", EntityDataType::ENTITY_DATA_TYPE_MAT4, 0, nullptr);
	terrain->m_binding_data.AddElement("model", EntityDataType::ENTITY_DATA_TYPE_MAT4, sizeof(mat4), nullptr);
	terrain->m_binding_data.AddElement("inverse_model", EntityDataType::ENTITY_DATA_TYPE_MAT4, sizeof(mat4) * 2, nullptr);
	terrain->m_binding_data.AddElement("projection", EntityDataType::ENTITY_DATA_TYPE_MAT4, sizeof(mat4) * 3, nullptr);

	terrain->m_input_description.AddInputElement("inPosition", 0, 0, 3, true, false, false);	
	terrain->m_input_description.AddInputElement("inNormal", 1, 0, 3, true, false, false);
	terrain->m_input_description.AddInputElement("inTextureIDs", 2, 0, 3, false, true, false);
	terrain->m_input_description.AddInputElement("inTextureWeights", 3, 0, 3, true, false, false);
	terrain->m_input_description.AddInputElement("inTexCoords", 4, 0, 2, true, false, false);

	return terrain;
}

void Terrain_Destroy(ITerrain terrain)
{
	terrain->m_render_state.Destroy();
	ShaderProgramData_Destroy(terrain->m_program_data);
	Buffer2_Destroy(terrain->m_vertices_buffer);
	Buffer2_Destroy(terrain->m_indices_buffer);
	delete terrain;
}