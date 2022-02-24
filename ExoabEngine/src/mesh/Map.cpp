#include "Map.hpp"
#include <iostream>

ITerrain Terrain_Create(GraphicsContext context, VkSampler sampler, int width, int xresolution, int height, int yresolution, ITexture2 basetexture, std::vector<ITexture2> textures)
{
	ITerrain terrain = new Terrain();
	width = (width % 2 == 0) ? width : width + 1;
	height = (height % 2 == 0) ? height : height + 1;
	xresolution = std::max(xresolution, 1);
	yresolution = std::max(yresolution, 1);
	terrain->m_context = context;
	terrain->m_width = width;
	terrain->m_height = height;
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

	terrain->m_vertices_ssbo = Buffer2_Create(terrain->m_context, BufferType::StorageBuffer, terrain->m_vertices.size() * sizeof(TerrainVertex), BufferMemoryType::GPU_ONLY);
	terrain->m_indices_buffer = Buffer2_Create(terrain->m_context, BufferType::IndexBuffer, terrain->m_indices.size() * sizeof(uint16_t), BufferMemoryType::GPU_ONLY);

	Buffer2_UploadData(terrain->m_vertices_ssbo, (char8_t*)terrain->m_vertices.data(), 0, terrain->m_vertices.size() * sizeof(TerrainVertex));
	Buffer2_UploadData(terrain->m_indices_buffer, (char8_t*)terrain->m_indices.data(), 0, terrain->m_indices.size() * sizeof(uint16_t));
	
	std::vector<ShaderBinding> vert_bindings(2);
	vert_bindings[0].m_type = SHADER_BINDING_SHADER_STORAGE_BUFFER_OBJECT;
	vert_bindings[0].m_bindingID = 0;
	vert_bindings[0].m_hostvisible = false;
	vert_bindings[0].m_useclientbuffer = true;
	vert_bindings[0].m_additional_buffer_flags = (BufferType)0;
	vert_bindings[0].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	vert_bindings[0].m_size = sizeof(TerrainVertex) * terrain->m_vertices.size();
	vert_bindings[0].m_client_buffer = terrain->m_vertices_ssbo;

	vert_bindings[1].m_type = SHADER_BINDING_UNIFORM_BUFFER;
	vert_bindings[1].m_bindingID = 1;
	vert_bindings[1].m_hostvisible = true;
	vert_bindings[1].m_useclientbuffer = false;
	vert_bindings[1].m_additional_buffer_flags = (BufferType)0;
	vert_bindings[1].m_shaderStages = VK_SHADER_STAGE_VERTEX_BIT;
	vert_bindings[1].m_size = sizeof(glm::mat4) * 4;

	std::vector<ShaderBinding> frag_bindings(2);
	frag_bindings[0].m_type = SHADER_BINDING_SAMPLER;
	frag_bindings[0].m_bindingID = 0;
	frag_bindings[0].m_shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_bindings[0].m_size = 0;
	frag_bindings[0].m_sampler.push_back(sampler);

	frag_bindings[1].m_type = SHADER_BINDING_TEXTURE;
	frag_bindings[1].m_bindingID = 1;
	frag_bindings[1].m_shaderStages = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_bindings[1].m_size = 0;
	frag_bindings[1].m_textures.push_back(basetexture);
	frag_bindings[1].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	for (int j = 0; j < textures.size(); j++) {
		frag_bindings[1].m_textures.push_back(textures[j]);
		frag_bindings[1].m_textures_layouts.push_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	uint32_t frameCount = ToVKContext(context)->FrameInfo->m_FrameCount;
	std::vector<VkDescriptorPoolSize> poolSize;
	ShaderBinding_CalculatePoolSizes(frameCount, poolSize, vert_bindings);
	ShaderBinding_CalculatePoolSizes(frameCount, poolSize, frag_bindings);

	terrain->m_pool = vk::Gfx_CreateDescriptorPool(ToVKContext(context), 2 * frameCount, poolSize);
	terrain->m_Set0 = ShaderBinding_Create(ToVKContext(context), terrain->m_pool, 0, vert_bindings);
	terrain->m_Set1 = ShaderBinding_Create(ToVKContext(context), terrain->m_pool, 1, frag_bindings);

	std::vector<VkPushConstantRange> ranges(1);
	ranges[0].offset = 0;
	ranges[0].size = sizeof(glm::vec3);
	ranges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	terrain->m_layout = ShaderBinding_CreatePipelineLayout(ToVKContext(context), { terrain->m_Set0, terrain->m_Set1 }, ranges);

	return terrain;
}

void Terrain_Destroy(ITerrain terrain)
{
	Buffer2_Destroy(terrain->m_vertices_ssbo);
	Buffer2_Destroy(terrain->m_indices_buffer);
	ShaderBinding_DestroySets(ToVKContext(terrain->m_context), { terrain->m_Set0, terrain->m_Set1 });
	vkDestroyDescriptorPool(ToVKContext(terrain->m_context)->defaultDevice, terrain->m_pool, nullptr);
	delete terrain;
}
