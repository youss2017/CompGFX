#include "Terrain.hpp"
#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/ProgressHandler.hpp>
#include "zip.h"

Terrain::Terrain(uint32_t width, uint32_t height, uint32_t splitX, uint32_t splitY) : mModelTransform(glm::mat4(1.0)) {
	using namespace glm;
	using namespace Ph;

	mWidth = width;
	mHeight = height;

	if (width % 2 == 0)
		splitX = (splitX % 2 == 0) ? splitX : splitX + 1;
	else
		splitX = (splitX % 3 == 0) ? splitX : splitX + 1;
	if(height % 2 == 0)
		splitY = (splitY % 2 == 0) ? splitY : splitY + 1;
	else
		splitY = (splitY % 3 == 0) ? splitY : splitY + 1;

	mSplitX = splitX;
	mSplitY = splitY;

	uint32_t firstVertex = 0;
	uint32_t firstIndex = 0;
	int YY = 0;
	for (uint32_t yBlock = 0; yBlock < height; yBlock += splitY) {
		for (uint32_t xBlock = 0; xBlock < width; xBlock += splitX) {
			uint32_t resolutionStepX = splitX + 1;
			uint32_t resolutionStepY = splitY + 1;
			TerrainSubmesh submesh;
			for (uint32_t y = 0; y < resolutionStepY; y++) {
				for (uint32_t x = 0; x < resolutionStepX; x++) {
					TerrainVertex v{};
					vec3 position = glm::vec3(x + xBlock, 1.0f, y + yBlock);
					v.inPosition16 = HalfVec3(position);
					v.inNormal16 = HalfVec3(glm::vec3(0.0, 1.0, 0.0));
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
			submesh.mFirstVertex = firstVertex;
			submesh.mFirstIndex = firstIndex;
			submesh.mIndicesCount = mIndices.size() - firstIndex;
			mSubmeshes.push_back(submesh);
			firstIndex = mIndices.size();
			firstVertex = mVertices.size();

		}
	}
	
	int submeshX = width / splitX;
	int submeshY = height / splitY;

	std::vector<TerrainSubmesh> ss;
	for (int y = 0; y < submeshY; y++) {
		for (int x = 0; x < submeshX; x++) {
			if (x != submeshX - 1 && y != submeshY - 1) {
				ss.push_back(mSubmeshes[y * submeshX + x]);
			}
		}
	}
	mSubmeshes = ss;
	printf("Total Vertices %llu Indices %llu Total submeshes: %llu\n", mVertices.size(), mIndices.size(), mSubmeshes.size());
	CalculateTangentBitangent();
	mVertices.shrink_to_fit();
	mIndices.shrink_to_fit();
	mVerticesBuffer = Buffer2_CreatePreInitalized(BufferType::BUFFER_TYPE_STORAGE, mVertices.data(), mVertices.size() * sizeof(TerrainVertex), BufferMemoryType::GPU_ONLY, true, false);
	mIndicesBuffer = Buffer2_CreatePreInitalized(BufferType::BUFFER_TYPE_INDEX, mIndices.data(), mIndices.size() * 4, BufferMemoryType::GPU_ONLY, true, false);
	mModelTransform = glm::translate(glm::mat4(1.0), glm::vec3(-(float(mWidth - mSplitX) / 2.0f), 0.0, -(float(mHeight - mSplitY) / 2.0f)));
}

Terrain::~Terrain() {
	Buffer2_Destroy(mVerticesBuffer);
	Buffer2_Destroy(mIndicesBuffer);
}

void Terrain::ApplyHeightmap(int heightMapWidth, int heightMapHeight, float minHeight, float maxHeight, std::vector<TerrainHeightMap> heightMaps) {
	using namespace glm;

	auto sample = [heightMapWidth, heightMapHeight, minHeight, maxHeight, heightMaps](float x, float y) throw() -> float {
		/*		KtK
				lcr
				KbK
			c = 0.5
			l = r = t = b = 0.1
			K1 = K2 = K3 = K4 = 0.025
		*/
		// wrap around x, y coordinates
		if (x > 1.0) {
			x = fract(x);
		}
		if (y > 1.0) {
			y = fract(y);
		}
		x = clamp<float>(x * (float)heightMapWidth, 0, heightMapWidth);
		y = clamp<float>(y * (float)heightMapHeight, 0, heightMapHeight);
		vec2 size = vec2(heightMapWidth, heightMapHeight);
		vec2 texel = 1.0f / size;

		// In Bounds returns 1.0 out of bounds returns 0.0
		auto outOfBounds = [=](ivec2* coord, float* boundStatus) throw() -> void {
			if (coord->x < 0 || coord->x >= heightMapWidth) {
				*coord = ivec2(0);
				*boundStatus = 0.0;
			}
			if (coord->y < 0 || coord->y >= heightMapHeight) {
				*coord = ivec2(0);
				*boundStatus = 0.0;
			}
			*boundStatus = 1.0;
		};

		vec2 c = vec2(x, y) / size;
		ivec2 l =  ivec2(size * (c - vec2(texel.x, 0.0)));
		ivec2 r =  ivec2(size * (c + vec2(texel.x, 0.0)));
		ivec2 t =  ivec2(size * (c - vec2(0.0, texel.y)));
		ivec2 b = ivec2(size * (c + vec2(0.0, texel.y)));
		ivec2 k1 = ivec2(size * (c - texel));
		ivec2 k2 = ivec2(size * (c + vec2(texel.x, -texel.y)));
		ivec2 k3 = ivec2(size * (c + vec2(-texel.x, texel.y)));
		ivec2 k4 = ivec2(size * (c + texel));

		ivec2 cc = c;
		float cBoundStatus{}; outOfBounds(&cc, &cBoundStatus);
		float lBoundStatus{};  outOfBounds(&l, &lBoundStatus);
		float rBoundStatus{};  outOfBounds(&r, &rBoundStatus);
		float tBoundStatus{};  outOfBounds(&t, &tBoundStatus);
		float bBoundStatus{};  outOfBounds(&b, &bBoundStatus);
		float k1BoundStatus{}; outOfBounds(&k1, &k1BoundStatus);
		float k2BoundStatus{}; outOfBounds(&k2, &k2BoundStatus);
		float k3BoundStatus{}; outOfBounds(&k3, &k3BoundStatus);
		float k4BoundStatus{}; outOfBounds(&k4, &k4BoundStatus);
		c = cc;

		float height = 0.0;
		for (auto& heightmap : heightMaps) {
			float cV  = 0.5f * (float(heightmap.mHeightMap[int(y) * heightMapWidth + int(x)]))        * cBoundStatus;
			float lV  = 0.1f * (float(heightmap.mHeightMap[l.y * heightMapWidth + l.x]))    * lBoundStatus;
			float rV  = 0.1f * (float(heightmap.mHeightMap[r.y * heightMapWidth + r.x]))    * rBoundStatus;
			float tV  = 0.1f * (float(heightmap.mHeightMap[t.y * heightMapWidth + t.x]))    * tBoundStatus;
			float bV  = 0.1f * (float(heightmap.mHeightMap[b.y * heightMapWidth + b.x]))    * bBoundStatus;
			float k1V = 0.025 * (float(heightmap.mHeightMap[k1.y * heightMapWidth + k1.x])) * k1BoundStatus;
			float k2V = 0.025 * (float(heightmap.mHeightMap[k2.y * heightMapWidth + k2.x])) * k2BoundStatus;
			float k3V = 0.025 * (float(heightmap.mHeightMap[k3.y * heightMapWidth + k3.x])) * k3BoundStatus;
			float k4V = 0.025 * (float(heightmap.mHeightMap[k4.y * heightMapWidth + k4.x])) * k4BoundStatus;
			float value = cV + lV + rV + tV + bV + k1V + k2V + k3V + k4V;
			height += mix(minHeight, maxHeight, value) * heightmap.mContribution;
		}
		return height;
	};

	using namespace Ph;
	for (auto& submesh : mSubmeshes) {
		TerrainVertex* vertices = &mVertices[submesh.mFirstVertex];
		uint32_t* indices = &mIndices[submesh.mFirstIndex];
		for (uint32_t i = 0; i < submesh.mIndicesCount; i++) {
			int x = FullFloat(vertices[indices[i]].inPosition16.x);
			int y = FullFloat(vertices[indices[i]].inPosition16.z);
			float X = (float(x) / float(mWidth));
			float Y = (float(y) / float(mHeight));
			vertices[indices[i]].inPosition16.y = HalfFloat(sample(X, Y));
		}
	}
	CalculateTangentBitangent();
	Buffer2_UploadData(mVerticesBuffer, mVertices.data(), 0, VK_WHOLE_SIZE);
}

glm::mat4 Terrain::GetToCenterTransform() {
	glm::mat4 center = glm::translate(glm::mat4(1.0), glm::vec3(-(float(mWidth - mSplitX) / 2.0f), 0.0, -(float(mHeight - mSplitY) / 2.0f)));
	return center;
}

glm::mat4 Terrain::GetSquareTransform(float maxHeight) {
	glm::mat4 scale = glm::scale(glm::mat4(1.0), glm::vec3(1.0f / float(mWidth - mSplitX), 1.0f / maxHeight, 1.0f / (float(mHeight - mSplitY))));
	return scale;
}

std::vector<std::pair<std::string, std::string>> Terrain::GetFilters(std::vector<std::string>& extensionIDs)
{
	std::vector<std::pair<std::string, std::string>> filters;
	Assimp::Exporter exp;
	for (int i = 0; i < exp.GetExportFormatCount(); i++) {
		const auto& desc = exp.GetExportFormatDescription(i);
		filters.push_back({desc->description, "*." + std::string(desc->fileExtension)});
		extensionIDs.push_back(desc->id);
	}
	return filters;
}

// Thanks to https://github.com/assimp/assimp/issues/203
void Terrain::Export(const std::string& assimpID, const std::string& path, size_t* pOutSize, char** pOutBuffer) {
	aiScene scene{};
	auto start1 = std::chrono::high_resolution_clock::now();
	scene.mRootNode = new aiNode();

	scene.mMaterials = new aiMaterial*[1];
	scene.mMaterials[0] = nullptr;
	scene.mNumMaterials = 1;

	scene.mMaterials[0] = new aiMaterial();

	scene.mMeshes = new aiMesh*[mSubmeshes.size()];
	scene.mMeshes[0] = nullptr;
	scene.mNumMeshes = mSubmeshes.size();
	scene.mRootNode->mMeshes = new unsigned int[mSubmeshes.size()];
	scene.mRootNode->mNumMeshes = mSubmeshes.size();
	glm::mat4 center = glm::translate(glm::mat4(1.0), glm::vec3(-(float(mWidth - mSplitX) / 2.0f), 0.0, -(float(mHeight - mSplitY) / 2.0f)));
	for (int ii = 0; ii < mSubmeshes.size(); ii++) {
		scene.mRootNode->mMeshes[ii] = ii;
		scene.mMeshes[ii] = new aiMesh();
		scene.mMeshes[ii]->mMaterialIndex = 0;
		scene.mMeshes[ii]->mName = "TerrainSubmeshID[" + std::to_string(ii) + "]";
		auto pMesh = scene.mMeshes[ii];

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> uvs;

		using namespace Ph;
		auto& submesh = mSubmeshes[ii];
		auto data = &mVertices[submesh.mFirstVertex];
		for (uint32_t i = 0; i < submesh.mIndicesCount; i++)
		{
			auto& v = data[mIndices[submesh.mFirstIndex + i]];
			vertices.push_back(glm::vec3(center * glm::vec4(FullVec3(v.inPosition16), 1.0f)));
			normals.push_back(FullVec3(v.inNormal16));
			//uvs.push_back(glm::vec2(Ph::FullVec2(v.inTexCoords)));
			uvs.push_back(glm::vec2(0.0));
		}

		const auto& vVertices = vertices;
		pMesh->mVertices = new aiVector3D[vVertices.size()];
		pMesh->mNormals = new aiVector3D[vVertices.size()];
		pMesh->mNumVertices = vVertices.size();
		pMesh->mTextureCoords[0] = new aiVector3D[vVertices.size()];
		pMesh->mNumUVComponents[0] = vVertices.size();
		int j = 0;
		for (auto itr = vVertices.begin(); itr != vVertices.end(); ++itr)
		{
			pMesh->mVertices[itr - vVertices.begin()] = aiVector3D(vVertices[j].x, vVertices[j].y, vVertices[j].z);
			pMesh->mNormals[itr - vVertices.begin()] = aiVector3D(normals[j].x, normals[j].y, normals[j].z);
			pMesh->mTextureCoords[0][itr - vVertices.begin()] = aiVector3D(uvs[j].x, uvs[j].y, 0);
			j++;
		}

		pMesh->mFaces = new aiFace[vVertices.size() / 3];
		pMesh->mNumFaces = (unsigned int)(vVertices.size() / 3);

		int k = 0;
		for (int i = 0; i < (vVertices.size() / 3); i++)
		{
			aiFace& face = pMesh->mFaces[i];
			face.mIndices = new unsigned int[3];
			face.mNumIndices = 3;

			face.mIndices[0] = k;
			face.mIndices[1] = k + 1;
			face.mIndices[2] = k + 2;
			k = k + 3;
		}
	}
	auto end1 = std::chrono::high_resolution_clock::now();
	
	auto start2 = std::chrono::high_resolution_clock::now();
	Assimp::Exporter exp;
	
	class Progress : public Assimp::ProgressHandler {
	public:
		bool Update(float percentage = -1.f) {
			printf("Exporting Progress %.2f\n", percentage * 100.0f);
			return true;
		}
	};
	Progress* p = new Progress;
	exp.SetProgressHandler(p);
	if (pOutSize) {
		auto buf = exp.ExportToBlob(&scene, assimpID);
		*pOutSize = buf->size;
		*pOutBuffer = new char[buf->size];
		memcpy(*pOutBuffer, buf->data, buf->size);
	}
	else {
		if (exp.Export(&scene, assimpID, path) != AI_SUCCESS) {
			logerror("Could not export terrain!");
		}
	}
	auto end2 = std::chrono::high_resolution_clock::now();
	printf("Copying Data to Assimp structure took %.2f sec\nAssimp Exporting took %.2f sec\n", float((end1 - start1).count()) * 1e-9f, float((end2 - start2).count()) * 1e-9f);
}

void Terrain::Save(const std::string& name) {
	std::vector<char*> files;
	zip_t* zip = zip_open(name.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
	zip_entry_open(zip, "MANIFEST.DAT");
	{
		using namespace std;
		string TerrainName = "TERRAIN:"s + name;
		zip_entry_write(zip, TerrainName.data(), TerrainName.size());
		string normalMap = "NORMAL:";
		zip_entry_write(zip, normalMap.data(), normalMap.size());
		string specularMap = "SPECULAR:";
		zip_entry_write(zip, specularMap.data(), specularMap.size());
		zip_entry_close(zip);
	}
	zip_entry_close(zip);
	zip_entry_open(zip, "terrain.fbx");
	char* pBuffer;
	size_t size;
	Export("fbx", "", &size, &pBuffer);
	zip_entry_write(zip, pBuffer, size);
	zip_entry_close(zip);
	zip_close(zip);
	delete[] pBuffer;
}

void Terrain::CalculateTangentBitangent() {
	using namespace Ph;
	for (auto& submesh : mSubmeshes) {
		auto vertices = &mVertices[submesh.mFirstVertex];
		auto indices = &mIndices[submesh.mFirstIndex];
		submesh.mSphere = Ph::CalculateBoundingSphere(vertices, indices, submesh.mIndicesCount, sizeof(TerrainVertex));
		for (uint32_t i = 0; i < submesh.mIndicesCount; i+=3) {
			auto T0 = &vertices[indices[i+0]];
			auto T1 = &vertices[indices[i+1]];
			auto T2 = &vertices[indices[i+2]];
			auto deltaPos1 = FullVec3(T1->inPosition16) - FullVec3(T0->inPosition16);
			auto deltaPos2 = FullVec3(T2->inPosition16) - FullVec3(T0->inPosition16);
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
			T0->inNormal16 = T1->inNormal16 = T2->inNormal16 = HalfVec3(glm::cross(deltaPos1, deltaPos2));
			T0->inTangent16 = T1->inTangent16 = T2->inTangent16 = HalfVec3(tangent * -1.0f);
			//T0->inBiTangent8 = T1->inBiTangent8 = T2->inBiTangent8 = Normal32To8(bitangent);
		}
	}
}
