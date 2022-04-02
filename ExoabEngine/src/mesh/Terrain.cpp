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
			TerrainSubmesh submesh;
			bool set = false;
			for (uint32_t y = 0; y < resolutionStepY; y++) {
				for (uint32_t x = 0; x < resolutionStepX; x++) {
					TerrainVertex v{};
					vec3 position = glm::vec3(x + xBlock, 1.0f, y + yBlock);
					v.inPosition = position;
					v.inNormal = HalfVec3(glm::vec3(0.0, 1.0, 0.0));
					v.inTexCoords = HalfVec2(glm::vec2(position.x / float(width), position.z / float(height)));
					mVertices.push_back(v);
					if (!set) {
						set = true;
						submesh.mBox.mBoxMin = position;
					}
					submesh.mBox.mBoxMax = position;
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
			submesh.mFirstVertex = firstVertex;
			submesh.mFirstIndex = firstIndex;
			submesh.mIndicesCount = mIndices.size() - firstIndex;
			mSubmeshes.push_back(submesh);
			firstIndex = mIndices.size();
			firstVertex = mVertices.size();

		}
	}
	printf("Total Vertices %llu Indices %llu Total submeshes: %llu\n", mVertices.size(), mIndices.size(), mSubmeshes.size());
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

void Terrain::ApplyHeightmap(int heightMapWidth, int heightMapHeight, float minHeight, float maxHeight, float* heightmap) {
	using namespace glm;

	auto sample = [heightMapWidth, heightMapHeight, minHeight, maxHeight, heightmap](int x, int y) throw() -> float {
		/*		KtK
				lcr
				KbK
			c = 0.5
			l = r = t = b = 0.1
			K1 = K2 = K3 = K4 = 0.025
		*/
		x = clamp(x, 0, heightMapWidth);
		y = clamp(y, 0, heightMapHeight);
		vec2 size = vec2(heightMapWidth, heightMapHeight);
		vec2 texel = 1.0f / size;
		vec2 c =   clamp(vec2(x, y) / size, vec2(0.0f), vec2(1.0f));
		ivec2 l =  clamp(ivec2(size * (c - vec2(texel.x, 0.0))), ivec2(0), ivec2(size));
		ivec2 r =  clamp(ivec2(size * (c + vec2(texel.x, 0.0))), ivec2(0), ivec2(size));
		ivec2 t =  clamp(ivec2(size * (c - vec2(0.0, texel.y))), ivec2(0), ivec2(size));
		ivec2 b =  clamp(ivec2(size * (c + vec2(0.0, texel.y))), ivec2(0), ivec2(size));
		ivec2 k1 = clamp(ivec2(size * (c - texel)), ivec2(0), ivec2(size));
		ivec2 k2 = clamp(ivec2(size * (c + vec2(texel.x, -texel.y))), ivec2(0), ivec2(size));
		ivec2 k3 = clamp(ivec2(size * (c + vec2(-texel.x, texel.y))), ivec2(0), ivec2(size));
		ivec2 k4 = clamp(ivec2(size * (c + texel)), ivec2(0), ivec2(size));
		float cV  = 0.5f * float(heightmap[y * heightMapWidth + x]);
		float lV  = 0.1f * float(heightmap[l.y * heightMapWidth + l.x]);
		float rV  = 0.1f * float(heightmap[r.y * heightMapWidth + r.x]);
		float tV  = 0.1f * float(heightmap[t.y * heightMapWidth + t.x]);
		float bV  = 0.1f * float(heightmap[b.y * heightMapWidth + b.x]);
		float k1V = 0.025 * float(heightmap[k1.y * heightMapWidth + k1.x]);
		float k2V = 0.025 * float(heightmap[k2.y * heightMapWidth + k2.x]);
		float k3V = 0.025 * float(heightmap[k3.y * heightMapWidth + k3.x]);
		float k4V = 0.025 * float(heightmap[k4.y * heightMapWidth + k4.x]);
		float value = clamp(cV + lV + rV + tV + bV + k1V + k2V + k3V + k4V, 0.0f, 1.0f);
		return mix(minHeight, maxHeight, value);
	};

	for(auto& submesh : mSubmeshes) {
		TerrainVertex* vertices = &mVertices[submesh.mFirstVertex];
		uint32_t* indices = &mIndices[submesh.mFirstIndex];
		for (int i = 0; i < submesh.mIndicesCount; i++) {
			int x = vertices[indices[i]].inPosition.x;
			int y = vertices[indices[i]].inPosition.z;
			int X = (float(mVertices[y * mWidth + x].inPosition.x) / float(mWidth)) * float(heightMapWidth);
			int Y = (float(mVertices[y * mWidth + x].inPosition.z) / float(mHeight)) * float(heightMapHeight);
			mVertices[y * mWidth + x].inPosition.y = sample(X, Y);
		}
	}
	CalculateTangentBitangent();
	Buffer2_UploadData(mVerticesBuffer, mVertices.data(), 0, VK_WHOLE_SIZE);
}

void Terrain::CalculateTangentBitangent() {
	using namespace Ph;
	for (auto& submesh : mSubmeshes) {
		auto vertices = &mVertices[submesh.mFirstVertex];
		auto indices = &mIndices[submesh.mFirstIndex];
		submesh.mBox = Ph::CalculateBoundingBox(vertices, indices, submesh.mIndicesCount, sizeof(TerrainVertex));
		for (uint32_t i = 0; i < submesh.mIndicesCount; i+=3) {
			auto T0 = &vertices[indices[i+0]];
			auto T1 = &vertices[indices[i+1]];
			auto T2 = &vertices[indices[i+2]];
			auto deltaPos1 = T1->inPosition - T0->inPosition;
			auto deltaPos2 = T2->inPosition - T0->inPosition;
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
	}
}
