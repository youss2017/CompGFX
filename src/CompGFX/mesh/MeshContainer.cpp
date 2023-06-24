#include "MeshContainer.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stdexcept>
#include <Utility/CppUtility.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace egx;
using namespace std;
using namespace glm;

MeshContainer& egx::MeshContainer::Load(const std::string& file, IndicesType type, const std::vector<VertexDataOrder>& vertexDataOrder)
{
	Assimp::Importer importer;
	auto scene = importer.ReadFile(file, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_CalcTangentSpace);
	if (!scene) {
		throw runtime_error(cpp::Format("Cannot load model file {} either not supported or not found.", file));
	}
	m_MeshData.clear(), m_MeshData.reserve(scene->mNumMeshes);
	m_IndicesType = type;
	size_t vertexSize = 0;
	for (const auto& vo : vertexDataOrder) {
		if (vo == VertexDataOrder::UV) {
			vertexSize += sizeof(float) * 2;
		}
		else {
			vertexSize += sizeof(float) * 3;
		}
	}
	for (uint32_t meshId = 0; meshId < scene->mNumMeshes; meshId++) {
		unique_ptr<MeshContainer::Mesh> mesh = make_unique<MeshContainer::Mesh>();
		bool containsUV = scene->mMeshes[meshId]->HasTextureCoordsName(0);
		auto& vertices = mesh->m_Vertices;
		vertices.resize(vertexSize * scene->mMeshes[meshId]->mNumVertices);
		for (uint32_t vertexId = 0; vertexId < scene->mMeshes[meshId]->mNumVertices; vertexId++) {
			size_t offset = (vertexSize / sizeof(float)) * vertexId;
			for (VertexDataOrder vo : vertexDataOrder) {
				// Position, Normal, UV, Tangent, Bitangent
				switch (vo) {
				case VertexDataOrder::Position:
					vertices[offset + 0] = scene->mMeshes[meshId]->mVertices[vertexId].x;
					vertices[offset + 1] = scene->mMeshes[meshId]->mVertices[vertexId].y;
					vertices[offset + 2] = scene->mMeshes[meshId]->mVertices[vertexId].z;
					offset += 3;
					break;
				case VertexDataOrder::Normal:
					vertices[offset + 0] = scene->mMeshes[meshId]->mNormals[vertexId].x;
					vertices[offset + 1] = scene->mMeshes[meshId]->mNormals[vertexId].y;
					vertices[offset + 2] = scene->mMeshes[meshId]->mNormals[vertexId].z;
					offset += 3;
					break;
				case VertexDataOrder::UV:
					if (containsUV) {
						vertices[offset + 0] = scene->mMeshes[meshId]->mNormals[vertexId].x;
						vertices[offset + 1] = scene->mMeshes[meshId]->mNormals[vertexId].y;
					}
					else {
						vertices[offset + 0] = 0.0f;
						vertices[offset + 1] = 0.0f;
					}
					offset += 2;
					break;
				case VertexDataOrder::Tangent:
					vertices[offset + 0] = scene->mMeshes[meshId]->mTangents[vertexId].x;
					vertices[offset + 1] = scene->mMeshes[meshId]->mTangents[vertexId].y;
					vertices[offset + 2] = scene->mMeshes[meshId]->mTangents[vertexId].z;
					offset += 3;
					break;
				case VertexDataOrder::Bitangent:
					vertices[offset + 0] = scene->mMeshes[meshId]->mBitangents[vertexId].x;
					vertices[offset + 1] = scene->mMeshes[meshId]->mBitangents[vertexId].y;
					vertices[offset + 2] = scene->mMeshes[meshId]->mBitangents[vertexId].z;
					offset += 3;
					break;
				default:
					LOG(WARNING, "Unknown VertexDataOrder ignored --- {}", uint32_t(vo));
				}
			}
		}

		// Load indices
		if (type == IndicesType::Int16) {
			auto& indices = mesh->m_Indice16;
			indices.resize(scene->mMeshes[meshId]->mNumFaces * 3ull);
			for (uint32_t faceId = 0, counter = 0; faceId < scene->mMeshes[meshId]->mNumFaces; faceId++) {
				indices[counter++] = (uint16_t)scene->mMeshes[meshId]->mFaces[faceId].mIndices[0];
				indices[counter++] = (uint16_t)scene->mMeshes[meshId]->mFaces[faceId].mIndices[1];
				indices[counter++] = (uint16_t)scene->mMeshes[meshId]->mFaces[faceId].mIndices[2];
			}
		}
		else {
			// Int32
			auto& indices = mesh->m_Indice32;
			indices.resize(scene->mMeshes[meshId]->mNumFaces * 3ull);
			for (uint32_t faceId = 0, counter = 0; faceId < scene->mMeshes[meshId]->mNumFaces; faceId++) {
				indices[counter++] = scene->mMeshes[meshId]->mFaces[faceId].mIndices[0];
				indices[counter++] = scene->mMeshes[meshId]->mFaces[faceId].mIndices[1];
				indices[counter++] = scene->mMeshes[meshId]->mFaces[faceId].mIndices[2];
			}
		}
		mesh->m_VerticesCount = (uint32_t)mesh->m_Vertices.size();
		mesh->m_IndicesCount = (uint32_t)std::max(mesh->m_Indice16.size(), mesh->m_Indice32.size());
		m_MeshData.push_back(move(mesh));
	}
	importer.FreeScene();
	return *this;
}

std::vector<float>& egx::MeshContainer::Vertices(uint32_t meshId) const
{
	return m_MeshData.at(meshId)->m_Vertices;
}

std::vector<uint32_t>& egx::MeshContainer::Indices32(uint32_t meshId) const
{
	return m_MeshData.at(meshId)->m_Indice32;
}

std::vector<uint16_t>& egx::MeshContainer::Indices16(uint32_t meshId) const
{
	return m_MeshData.at(meshId)->m_Indice16;
}

IndicesType egx::MeshContainer::GetIndicesType() const
{
	return m_IndicesType;
}

uint32_t egx::MeshContainer::MeshCount() const
{
	return (uint32_t)m_MeshData.size();
}

uint32_t egx::MeshContainer::GetVerticesCount(uint32_t meshId) const
{
	return m_MeshData[meshId]->m_VerticesCount;
}

uint32_t egx::MeshContainer::GetIndicesCount(uint32_t meshId) const
{
	return m_MeshData[meshId]->m_IndicesCount;
}

MeshContainer& egx::BufferedMeshContainer::Load(const std::string& file, IndicesType type, const std::vector<VertexDataOrder>& vertexDataOrder)
{
	if (m_Data.get() == nullptr) {
		throw runtime_error("You must call the constructor with DeviceCtx before calling load.");
	}
	MeshContainer::Load(file, type, vertexDataOrder);
	m_Data->m_VertexBuffers.clear(), m_Data->m_IndexBuffers.clear();
	m_Data->m_VertexBuffers.reserve(m_MeshData.size()), m_Data->m_IndexBuffers.reserve(m_MeshData.size());

	for (auto& mesh : m_MeshData) {
		Buffer vertexBuffer(m_Data->m_Ctx, mesh->m_Vertices.size() * sizeof(float), MemoryPreset::DeviceOnly, HostMemoryAccess::None, vk::BufferUsageFlagBits::eVertexBuffer, false);
		vertexBuffer.Write(mesh->m_Vertices.data());
		m_Data->m_VertexBuffers.push_back(vertexBuffer);

		if (m_IndicesType == IndicesType::Int16) {
			Buffer indexBuffer(m_Data->m_Ctx, mesh->m_Indice16.size() * sizeof(uint16_t), MemoryPreset::DeviceOnly, HostMemoryAccess::None, vk::BufferUsageFlagBits::eIndexBuffer, false);
			indexBuffer.Write(mesh->m_Indice16.data());
			m_Data->m_IndexBuffers.push_back(indexBuffer);
		}
		else {
			Buffer indexBuffer(m_Data->m_Ctx, mesh->m_Indice32.size() * sizeof(uint32_t), MemoryPreset::DeviceOnly, HostMemoryAccess::None, vk::BufferUsageFlagBits::eIndexBuffer, false);
			indexBuffer.Write(mesh->m_Indice32.data());
			m_Data->m_IndexBuffers.push_back(indexBuffer);
		}
	}

	return *this;
}

Buffer egx::BufferedMeshContainer::GetVertexBuffer(uint32_t id) const
{
	return m_Data->m_VertexBuffers.at(id);
}

Buffer egx::BufferedMeshContainer::GetIndexBuffer(uint32_t id) const
{
	return m_Data->m_IndexBuffers.at(id);
}

void egx::BufferedMeshContainer::ReleaseCPUData()
{
	for (auto& item : m_MeshData) {
		item->m_Vertices.clear(), item->m_Indice16.clear(), item->m_Indice32.clear();
	}
}

void egx::ModelContainer::UpdateTransform()
{
	// (TODO) Fix transform
	Transform = translate(mat4(1.0), Position) * rotate(mat4(1.0), 0.0f, vec3(0.0, 1.0, 0.0)) * scale(mat4(1.0), Scaling);
}
