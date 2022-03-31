#include "Terrain.hpp"
#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>

Terrain::Terrain(uint32_t resolution, uint32_t splitEveryAmountOfVerts) : mModelTransform(glm::mat4(1.0)), mResolution(resolution) {
	assert(resolution > 1);
	using namespace glm;

	uint32_t firstVertex = 0;
	uint32_t firstIndex = 0;
	for (int yBlock = 0; yBlock < resolution; yBlock += resolution / 2) {
		for (int xBlock = 0; xBlock < resolution; xBlock += resolution / 2) {
			uint32_t resolutionStep = resolution / 2 + 1;
			for (uint32_t y = 0; y < resolutionStep; y++) {
				for (uint32_t x = 0; x < resolutionStep; x++) {
					TerrainVertex v{};
					v.inPosition = glm::vec3(x + xBlock, 1.0f, y + yBlock);
					v.inNormal = glm::vec3(0.0, -1.0, 0.0);
					v.inTexCoords = glm::vec2(v.inPosition.x / float(resolutionStep), v.inPosition.z / float(resolutionStep));
					mVertices.push_back(v);
				}
			}

			for (uint32_t y = 0; y < resolutionStep; y++) {
				for (uint32_t x = 0; x < resolutionStep; x++) {
					printf("%d\n", x);
					// generate indices
					if (x != resolutionStep - 1 && y != resolutionStep - 1) {
						uint32_t indexA = y * resolutionStep + x;
						uint32_t indexB = indexA + 1;
						uint32_t indexC = (y + 1) * resolutionStep + x;
						uint32_t indexD = indexC + 1;
						mIndices.push_back(indexA);
						mIndices.push_back(indexB);
						mIndices.push_back(indexC);

						mIndices.push_back(indexB);
						mIndices.push_back(indexD);
						mIndices.push_back(indexC);
					}
				}
			}
			TerrainSubmesh submesh;
			submesh.mFirstVertex = firstVertex;
			submesh.mFirstIndex = firstIndex;
			submesh.mIndicesCount = mIndices.size() - firstIndex;
			mSubmeshes.push_back(submesh);
			firstIndex = mIndices.size();
			firstVertex = mVertices.size();
		}
	}


	// Break up the mesh
	
	printf("Total Vertices %llu Indices %llu\n", mVertices.size(), mIndices.size());
	CalculateTangentBitangent();
	mVertices.shrink_to_fit();
	mIndices.shrink_to_fit();
	mVerticesBuffer = Buffer2_CreatePreInitalized(BufferType::BUFFER_TYPE_VERTEX, mVertices.data(), mVertices.size() * sizeof(TerrainVertex), BufferMemoryType::GPU_ONLY, true, false);
	mIndicesBuffer = Buffer2_CreatePreInitalized(BufferType::BUFFER_TYPE_INDEX, mIndices.data(), mIndices.size() * 4, BufferMemoryType::GPU_ONLY, true, false);
}

Terrain::~Terrain() {
	Buffer2_Destroy(mVerticesBuffer);
	Buffer2_Destroy(mIndicesBuffer);
}

void Terrain::ApplyHeightmap(int heightMapWidth, int heightMapHeight, float minHeight, float maxHeight, uint8_t* heightmap) {
	using namespace glm;
	for (int mapY = 0; mapY < heightMapHeight; mapY++) {
		float yratio = float(mapY) / float(heightMapHeight);
		for (int mapX = 0; mapX < heightMapWidth; mapX++) {
			float height = clamp(float(heightmap[mapY * heightMapWidth + mapX]) / 255.0f, minHeight, maxHeight);
			float xratio = float(mapX) / float(heightMapWidth);
			mVertices[int(float(mResolution) * yratio + (float(mResolution) * xratio))].inPosition.y = height;
		}
	}
	CalculateTangentBitangent();
	Buffer2_UploadData(mVerticesBuffer, mVertices.data(), 0, VK_WHOLE_SIZE);
}

void Terrain::CalculateTangentBitangent() {
	auto vertices = mVertices.data();
	auto indices = mIndices.data();
	for (int i = 0; i < mIndices.size() / 3; i++) {
		auto T0 = &vertices[indices[i++]];
		auto T1 = &vertices[indices[i++]];
		auto T2 = &vertices[indices[i++]];
		auto deltaPos1 = T1->inPosition - T0->inPosition;
		auto deltaPos2 = T2->inPosition - T0->inPosition;
		auto deltaUV1  = T1->inTexCoords - T0->inTexCoords;
		auto deltaUV2  = T2->inTexCoords - T0->inTexCoords;
		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		auto tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent;
		bitangent.x = r * (-deltaUV2.x * deltaPos1.x + deltaUV1.x * deltaPos2.x);
		bitangent.y = r * (-deltaUV2.x * deltaPos1.y + deltaUV1.x * deltaPos2.y);
		bitangent.z = r * (-deltaUV2.x * deltaPos1.z + deltaUV1.x * deltaPos2.z);
		tangent = glm::normalize(tangent);
		bitangent = glm::normalize(bitangent);
		T0->inNormal = T1->inNormal = T2->inNormal = glm::cross(deltaPos1, deltaPos2);
		T0->inTangent = T1->inTangent = T2->inTangent = -1.0f * tangent;
		T0->inBiTangent = T1->inBiTangent = T2->inBiTangent = bitangent;
	}
}
