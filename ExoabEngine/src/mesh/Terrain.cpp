#include "Terrain.hpp"
#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>

Terrain::Terrain(uint32_t width, uint32_t height, uint32_t splitX, uint32_t splitY) : mModelTransform(glm::mat4(1.0)), mWidth(width), mHeight(height) {
	using namespace glm;
	using namespace Ph;

	if (width % 2 == 0)
		splitX = (splitX % 2 == 0) ? splitX : splitX + 1;
	else
		splitX = (splitX % 3 == 0) ? splitX : splitX + 1;
	if(height % 2 == 0)
		splitY = (splitY % 2 == 0) ? splitY : splitY + 1;
	else
		splitY = (splitY % 3 == 0) ? splitY : splitY + 1;

	uint32_t firstVertex = 0;
	uint32_t firstIndex = 0;
	for (uint32_t yBlock = 0; yBlock < height; yBlock += height / splitY) {
		for (uint32_t xBlock = 0; xBlock < width; xBlock += width / splitX) {
			uint32_t resolutionStepX = width / splitX + 1;
			uint32_t resolutionStepY = height / splitY + 1;
			for (uint32_t y = 0; y < resolutionStepY; y++) {
				for (uint32_t x = 0; x < resolutionStepX; x++) {
					TerrainVertex v{};
					vec3 position = glm::vec3(x + xBlock, 1.0f, y + yBlock);
					v.inPosition = HalfVec3(position);
					v.inNormal = HalfVec3(glm::vec3(0.0, 1.0, 0.0));
					v.inTexCoords = HalfVec2(glm::vec2(position.x / float(width), position.z / float(height)));
					mVertices.push_back(v);
				}
			}

			for (uint32_t y = 0; y < resolutionStepY; y++) {
				for (uint32_t x = 0; x < resolutionStepX; x++) {
					// generate indices
					if (x != resolutionStepX - 1 && y != resolutionStepY - 1) {
						uint32_t indexA = y * resolutionStepX + x;
						uint32_t indexB = indexA + 1;
						uint32_t indexC = (y + 1) * resolutionStepX + x;
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
	printf("Total Vertices %llu Indices %llu\n", mVertices.size(), mIndices.size());
	CalculateTangentBitangent();
	mVertices.shrink_to_fit();
	mIndices.shrink_to_fit();
	mVerticesBuffer = Buffer2_CreatePreInitalized(BufferType::BUFFER_TYPE_STORAGE, mVertices.data(), mVertices.size() * sizeof(TerrainVertex), BufferMemoryType::GPU_ONLY, true, false);
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
			mVertices[int((float(mHeight) * yratio) * float(mWidth) + (float(mWidth) * xratio))].inPosition.y = height;
		}
	}
	CalculateTangentBitangent();
	Buffer2_UploadData(mVerticesBuffer, mVertices.data(), 0, VK_WHOLE_SIZE);
}

void Terrain::CalculateTangentBitangent() {
	using namespace Ph;
	auto vertices = mVertices.data();
	auto indices = mIndices.data();
	for (auto& submesh : mSubmeshes) {
		for (uint32_t i = 0; i < submesh.mIndicesCount; i+=3) {
			auto T0 = &vertices[indices[i+0 + submesh.mFirstIndex] + submesh.mFirstVertex];
			auto T1 = &vertices[indices[i+1 + submesh.mFirstIndex] + submesh.mFirstVertex];
			auto T2 = &vertices[indices[i+2 + submesh.mFirstIndex] + submesh.mFirstVertex];
			auto deltaPos1 = FullVec3(T1->inPosition) - FullVec3(T0->inPosition);
			auto deltaPos2 = FullVec3(T2->inPosition) - FullVec3(T0->inPosition);
			auto deltaUV1  = FullVec2(T1->inTexCoords) - FullVec2(T0->inTexCoords);
			auto deltaUV2  = FullVec2(T2->inTexCoords) - FullVec2(T0->inTexCoords);
			float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
			auto tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
			glm::vec3 bitangent;
			bitangent.x = r * (-deltaUV2.x * deltaPos1.x + deltaUV1.x * deltaPos2.x);
			bitangent.y = r * (-deltaUV2.x * deltaPos1.y + deltaUV1.x * deltaPos2.y);
			bitangent.z = r * (-deltaUV2.x * deltaPos1.z + deltaUV1.x * deltaPos2.z);
			tangent = glm::normalize(tangent);
			bitangent = glm::normalize(bitangent);
			T0->inNormal = T1->inNormal = T2->inNormal = HalfVec3(glm::cross(deltaPos1, deltaPos2));
			T0->inTangent = T1->inTangent = T2->inTangent = HalfVec3(tangent * -1.0f);
			T0->inBiTangent = T1->inBiTangent = T2->inBiTangent = HalfVec3(bitangent);
		}
		submesh.mBox = Ph::CalculateBoundingBox16(vertices + submesh.mFirstVertex, indices, submesh.mIndicesCount, sizeof(TerrainVertex));
	}
}
